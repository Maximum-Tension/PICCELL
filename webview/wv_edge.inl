/* ---------------------------------------------------------------------------
 * wv_edge.inl - the WebView2 (Chromium) backend
 *
 * COM callback objects, environment and controller creation, script
 * evaluation with and without a result.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

/* webview_set_app_name() and getUserData() live in wv_paths.inl now. */

#define WEBVIEW2_WIN32_API extern

typedef void (*WebView2CallbackFn) (webview2 *wv, const char *message, void *context);

#define GET_AVAILABLE_COREWEBVIEW2_BROWSER_VERSION_FN_NAME "GetAvailableCoreWebView2BrowserVersionString"
#define CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME "CreateCoreWebView2EnvironmentWithOptions"

/* STDAPI exports: __stdcall. Without it a 32-bit build corrupts the stack
   on the first call (x64 has only one calling convention, so it never showed). */
typedef HRESULT (STDAPICALLTYPE *GetWebView2BrowserVersionInfoFnType) (PCWSTR browserExecutableFolder, LPWSTR* versionInfo);
typedef HRESULT (STDAPICALLTYPE *CreateCoreWebView2EnvironmentWithOptionsFnType) (PCWSTR browserExecutableFolder, PCWSTR userDataFolder, ICoreWebView2EnvironmentOptions* environmentOptions, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environment_created_handler);

typedef struct webview2_struct {
  HWND hwnd;
  const char *url;
  int debug;
  WebView2CallbackFn cb;
  void *context;
  ICoreWebView2 *webview;
  unsigned char ready;
  char *jsToEval;
  char *htmlToSet; /* webview_set_html() before the controller existed */
  int transparent; /* webview_set_transparent() likewise */
  ICoreWebView2Controller *controller;
  /* kept so <webview> guests can create their own controllers from the same
     browser process (see wv_guest.inl) */
  ICoreWebView2Environment *env;
  /* URL of the document that sent the message being handled */
  char message_source[512];
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler env_created_handler;
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl env_created_handler_vtbl;
  ICoreWebView2CreateCoreWebView2ControllerCompletedHandler controller_created_handler;
  ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl controller_created_handler_vtbl;
  ICoreWebView2WebMessageReceivedEventHandler message_received_event_handler;
  ICoreWebView2WebMessageReceivedEventHandlerVtbl message_received_event_handler_vtbl;
  ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler add_script_on_document_created_handler;
  ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandlerVtbl add_script_on_document_created_handler_vtbl;
  /* element.requestFullscreen() in the page, see webview_html_fullscreen_changed */
  ICoreWebView2ContainsFullScreenElementChangedEventHandler fullscreen_handler;
  ICoreWebView2ContainsFullScreenElementChangedEventHandlerVtbl fullscreen_handler_vtbl;
  /* Esc while the page is fullscreen */
  ICoreWebView2AcceleratorKeyPressedEventHandler accelerator_handler;
  ICoreWebView2AcceleratorKeyPressedEventHandlerVtbl accelerator_handler_vtbl;
} webview2;

#define WEBVIEW2_PTR_FROM(_cp, _field) \
	((webview2 *) ((char *) (_cp) - offsetof(webview2, _field)))

static CreateCoreWebView2EnvironmentWithOptionsFnType CreateCoreWebView2EnvironmentFn = NULL;

static int WebView2SetBackground(webview2 *pwv2, BYTE r, BYTE g, BYTE b,
                                 BYTE a);
static int WebView2SetTransparent(webview2 *pwv2, int transparent);

/* One short lived object per ExecuteScript() call, so several evaluations
   can be in flight at the same time. */
typedef void (*WebView2EvalCallbackFn)(void *userdata, int failed,
                                       const char *json);

typedef struct {
  ICoreWebView2ExecuteScriptCompletedHandler handler; /* must stay first */
  ICoreWebView2ExecuteScriptCompletedHandlerVtbl vtbl;
  WebView2EvalCallbackFn cb;
  void *userdata;
  HWND hwnd;
  LONG refcount;
} webview2_eval_ctx;

static HRESULT STDMETHODCALLTYPE WebView2WebMessageReceivedEventHandleInvoke(ICoreWebView2WebMessageReceivedEventHandler * This, ICoreWebView2 *webView, ICoreWebView2WebMessageReceivedEventArgs *args) {
  LPWSTR webMessage = NULL;
  LPWSTR source = NULL;
  HRESULT hRes;
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, message_received_event_handler);
  hRes = args->lpVtbl->TryGetWebMessageAsString(args, &webMessage);
  if (FAILED(hRes) || (webMessage == NULL)) {
    webMessage = NULL;
    hRes = args->lpVtbl->get_WebMessageAsJson(args, &webMessage);
  }
  if (FAILED(hRes) || (webMessage == NULL)) {
    return S_OK;
  }
  /* who sent it - the <webview> tag only obeys pages it trusts */
  if (SUCCEEDED(args->lpVtbl->get_Source(args, &source)) && (source != NULL)) {
    char *src = webview_from_utf16(source);
    CoTaskMemFree(source);
    if (src != NULL) {
      snprintf(pwv2->message_source, sizeof(pwv2->message_source), "%s", src);
      GlobalFree(src);
    }
  } else {
    pwv2->message_source[0] = '\0';
  }
  if (pwv2->cb != NULL) {
    char *message = webview_from_utf16(webMessage);
    if (message != NULL) {
      pwv2->cb(pwv2, message, pwv2->context);
      GlobalFree(message);
    }
  }
  /* This string belongs to us. It was never freed, which leaked every
     message - and the <webview> tag sends one per frame while scrolling. */
  CoTaskMemFree(webMessage);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE AddScriptToExecuteOnDocumentCreated_Invoke(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This, HRESULT errorCode, LPCWSTR id) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, add_script_on_document_created_handler);
  webview_print_log("AddScriptToExecuteOnDocumentCreated");
  pwv2->ready = 1;
  PostMessageW(pwv2->hwnd, WM_WEBVIEW_READY, 0, 0);
  return S_OK;
}

/* ------------------------------------------------------------------------
 * HTML fullscreen
 *
 * When a page calls element.requestFullscreen() - a video's fullscreen
 * button, a game, a slide deck - WebView2 does only half of it: the element
 * grows to fill the WebView, and the WebView stays exactly as big as it
 * was. Making the WINDOW fullscreen is left to the host, through this event.
 * So without it, "fullscreen" just meant "fills the window you already had".
 * ------------------------------------------------------------------------*/
static HRESULT STDMETHODCALLTYPE WebView2FullScreen_Invoke(
    ICoreWebView2ContainsFullScreenElementChangedEventHandler *This,
    ICoreWebView2 *sender, IUnknown *args) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, fullscreen_handler);
  struct webview *w = (struct webview *)pwv2->context;
  BOOL contains = FALSE;
  if ((w == NULL) || (sender == NULL)) {
    return S_OK;
  }
  sender->lpVtbl->get_ContainsFullScreenElement(sender, &contains);
  w->priv.page_fullscreen = contains ? 1 : 0;
  webview_print_log(contains ? "page entered fullscreen" : "page left fullscreen");
  webview_html_fullscreen_changed(w);
  return S_OK;
}

/* Leave HTML fullscreen in one page, if it is in it. exitFullscreen() needs
   no user gesture, unlike requestFullscreen(). */
#define WEBVIEW_JS_EXIT_FULLSCREEN                                             \
  "if(document.fullscreenElement&&document.exitFullscreen)"                    \
  "document.exitFullscreen().catch(function(){});"

static int WebView2ExecuteOn(ICoreWebView2 *view, HWND hwnd, const char *js,
                             WebView2EvalCallbackFn cb, void *userdata);

/* Esc leaves fullscreen, the way it does in every browser. Done here rather
   than trusted to the runtime, so it works the same on every WebView2
   version and even with w.browser.accelerator_keys turned off. Shared with
   the <webview> guests: returns 1 when it took the key. */
static int WebView2EscapeFromFullscreen(ICoreWebView2Controller *controller,
                                        HWND hwnd,
                                        ICoreWebView2AcceleratorKeyPressedEventArgs *args) {
  COREWEBVIEW2_KEY_EVENT_KIND kind = COREWEBVIEW2_KEY_EVENT_KIND_KEY_UP;
  ICoreWebView2 *view = NULL;
  BOOL contains = FALSE;
  UINT key = 0;
  if ((controller == NULL) || (args == NULL)) {
    return 0;
  }
  args->lpVtbl->get_KeyEventKind(args, &kind);
  args->lpVtbl->get_VirtualKey(args, &key);
  if ((key != VK_ESCAPE) || (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN)) {
    return 0;
  }
  if (FAILED(controller->lpVtbl->get_CoreWebView2(controller, &view)) ||
      (view == NULL)) {
    return 0;
  }
  view->lpVtbl->get_ContainsFullScreenElement(view, &contains);
  if (contains) {
    COREWEBVIEW2_PHYSICAL_KEY_STATUS status;
    ZeroMemory(&status, sizeof(status));
    args->lpVtbl->get_PhysicalKeyStatus(args, &status);
    /* the page does not see this Esc, as in Chrome and Edge */
    args->lpVtbl->put_Handled(args, TRUE);
    /* only the first press: holding Esc down repeats, and a second exit
       would take a nested fullscreen out one level too far */
    if (!status.WasKeyDown) {
      WebView2ExecuteOn(view, hwnd, WEBVIEW_JS_EXIT_FULLSCREEN, NULL, NULL);
    }
  }
  view->lpVtbl->Release(view);
  return contains ? 1 : 0;
}

static HRESULT STDMETHODCALLTYPE WebView2AcceleratorKey_Invoke(
    ICoreWebView2AcceleratorKeyPressedEventHandler *This,
    ICoreWebView2Controller *sender,
    ICoreWebView2AcceleratorKeyPressedEventArgs *args) {
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, accelerator_handler);
  WebView2EscapeFromFullscreen(sender, pwv2->hwnd, args);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE CreateWebView2Controller_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This, HRESULT result, ICoreWebView2Controller *webViewController) {
  ICoreWebView2 *webview;
  if (FAILED(result) || (webViewController == NULL)) {
    char buffer[256];
    sprintf(buffer, "CreateWebView2Controller_Invoke(%p, %08X, %p) => %08X (fail)", This, result, webViewController, E_FAIL);
    webview_print_log(buffer);
    return E_FAIL;
  }
  webview_print_log("CreateWebView2Controller_Invoke()");
  webViewController->lpVtbl->AddRef(webViewController);

  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, controller_created_handler);
  pwv2->controller = webViewController;

  webViewController->lpVtbl->get_CoreWebView2(webViewController, &webview);
  pwv2->webview = webview;

  ICoreWebView2Settings* settings;
  /* w.browser.* - devtools falls back to w.debug, everything else is left
     at WebView2's own default unless it was explicitly set. */
  if (pwv2->context != NULL) {
    webview_apply_browser_options((struct webview *)pwv2->context);
  } else {
    webview->lpVtbl->get_Settings(webview, &settings);
    if (settings->lpVtbl != NULL) {
      settings->lpVtbl->put_AreDevToolsEnabled(settings, pwv2->debug);
    }
  }

  RECT bounds;
  GetClientRect(pwv2->hwnd, &bounds);
  webViewController->lpVtbl->put_Bounds(webViewController, bounds);

  if (pwv2->transparent) {
    WebView2SetTransparent(pwv2, 1);
  }

  /* anything bound before the browser existed goes in now */
  if (pwv2->context != NULL) {
    webview_install_bindings((struct webview *)pwv2->context);
  }

  {
    /* window.external.invoke (C <- JS) plus the window.webview event bus
       (C -> JS). Injected before any page script runs, so a page can call
       window.webview.on(...) at the top of its own script. */
    wchar_t *wbootstrap = webview_to_utf16(WEBVIEW_JS_BOOTSTRAP);
    if (wbootstrap != NULL) {
      webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(
          webview, wbootstrap, &pwv2->add_script_on_document_created_handler);
      GlobalFree(wbootstrap);
    }
  }
  /* the <webview> tag, see wv_guest.inl */
  webview_guest_install_host_script(pwv2);

  EventRegistrationToken token;
  webview->lpVtbl->add_WebMessageReceived(webview, &pwv2->message_received_event_handler, &token);
  /* requestFullscreen() in the page -> the window goes fullscreen with it */
  webview->lpVtbl->add_ContainsFullScreenElementChanged(webview, &pwv2->fullscreen_handler, &token);
  webViewController->lpVtbl->add_AcceleratorKeyPressed(webViewController, &pwv2->accelerator_handler, &token);

  if (pwv2->htmlToSet != NULL) {
    /* webview_set_html() was called before this callback ran - which is the
       normal case, since CreateWebView2 only *starts* the creation. */
    wchar_t *whtml = webview_to_utf16(pwv2->htmlToSet);
    if (whtml != NULL) {
      webview_print_log("WebView2 NavigateToString()");
      webview->lpVtbl->NavigateToString(webview, whtml);
      GlobalFree(whtml);
    }
    free(pwv2->htmlToSet);
    pwv2->htmlToSet = NULL;
  } else if ((pwv2->url != NULL) && (strlen(pwv2->url) > 0)) {
    wchar_t *wurl = webview_to_utf16(pwv2->url);
    if (wurl != NULL) {
      webview_print_log("WebView2 Navigate()");
      HRESULT hr = webview->lpVtbl->Navigate(webview, wurl);
      GlobalFree(wurl);
      if (FAILED(hr)) { // E_INVALIDARG
        char buffer[256];
        sprintf(buffer, "Navigation failed with %08X on URL:", hr);
        webview_print_log(buffer);
        webview_print_log(pwv2->url);
      }
    }
  }

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE CreateWebView2Environment_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This, HRESULT result, ICoreWebView2Environment * webViewEnvironment) {
  webview_print_log("CreateWebView2Environment_Invoke()");
  if (FAILED(result) || (webViewEnvironment == NULL)) {
    char buffer[256];
    sprintf(buffer, "CreateWebView2Environment_Invoke(%p, %08X, %p) => %08X (fail)", This, result, webViewEnvironment, E_FAIL);
    webview_print_log(buffer);
    return E_FAIL;
  }
  webViewEnvironment->lpVtbl->AddRef(webViewEnvironment);
  webview2 *pwv2 = WEBVIEW2_PTR_FROM(This, env_created_handler);
  pwv2->env = webViewEnvironment; /* the AddRef above is this reference */
  webview_print_log("CreateWebView2Environment_Invoke() CreateCoreWebView2Controller");
  HRESULT hResult = webViewEnvironment->lpVtbl->CreateCoreWebView2Controller(webViewEnvironment, pwv2->hwnd, &pwv2->controller_created_handler);
  webview_print_log("CreateWebView2Environment_Invoke() => ok");
  return S_OK;
}

/* These handlers live inside the webview2 struct and are never freed on
   their own, so AddRef/Release have nothing to count. They used to be
   declared without STDMETHODCALLTYPE: harmless on x64, where there is one
   calling convention, but on x86 WebView2 calls them as __stdcall and a
   __cdecl callee leaves the arguments on the stack - the stack pointer drifts
   a little further on every message. QueryInterface also used to say S_OK
   without writing *ppvObject. It now answers for IUnknown and for the four
   handler interfaces this struct implements (each object is only ever asked
   about its own), and says E_NOINTERFACE to anything else - IMarshal,
   IAgileObject - which is what an in-process handler should say. */
static HRESULT STDMETHODCALLTYPE NoOpQueryInterface(void * This, REFIID riid, void **ppvObject) {
  if (ppvObject == NULL) {
    return E_POINTER;
  }
  if (iid_eq(riid, &IID_IUnknown) ||
      iid_eq(riid, &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) ||
      iid_eq(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) ||
      iid_eq(riid, &IID_ICoreWebView2WebMessageReceivedEventHandler) ||
      iid_eq(riid, &IID_ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler) ||
      iid_eq(riid, &IID_ICoreWebView2ContainsFullScreenElementChangedEventHandler) ||
      iid_eq(riid, &IID_ICoreWebView2AcceleratorKeyPressedEventHandler)) {
    *ppvObject = This;
    return S_OK;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE NoOpAddRef(void * This) {
  return 1;
}
static ULONG STDMETHODCALLTYPE NoOpRelease(void * This) {
  return 1;
}

static void InitWebView2(webview2 *pwv2) {
  pwv2->env_created_handler_vtbl.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->env_created_handler_vtbl.AddRef = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This)) &NoOpAddRef;
  pwv2->env_created_handler_vtbl.Release = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler * This)) &NoOpRelease;
  pwv2->env_created_handler_vtbl.Invoke = &CreateWebView2Environment_Invoke;
  pwv2->env_created_handler.lpVtbl = &pwv2->env_created_handler_vtbl;
  pwv2->controller_created_handler_vtbl.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->controller_created_handler_vtbl.AddRef = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This)) &NoOpAddRef;
  pwv2->controller_created_handler_vtbl.Release = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler * This)) &NoOpRelease;
  pwv2->controller_created_handler_vtbl.Invoke = &CreateWebView2Controller_Invoke;
  pwv2->controller_created_handler.lpVtbl = &pwv2->controller_created_handler_vtbl;
  pwv2->message_received_event_handler_vtbl.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(ICoreWebView2WebMessageReceivedEventHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->message_received_event_handler_vtbl.AddRef = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2WebMessageReceivedEventHandler * This)) &NoOpAddRef;
  pwv2->message_received_event_handler_vtbl.Release = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2WebMessageReceivedEventHandler * This)) &NoOpRelease;
  pwv2->message_received_event_handler_vtbl.Invoke = &WebView2WebMessageReceivedEventHandleInvoke;
  pwv2->message_received_event_handler.lpVtbl = &pwv2->message_received_event_handler_vtbl;
  pwv2->add_script_on_document_created_handler_vtbl.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->add_script_on_document_created_handler_vtbl.AddRef = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This)) &NoOpAddRef;
  pwv2->add_script_on_document_created_handler_vtbl.Release = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler * This)) &NoOpRelease;
  pwv2->add_script_on_document_created_handler_vtbl.Invoke = &AddScriptToExecuteOnDocumentCreated_Invoke;
  pwv2->add_script_on_document_created_handler.lpVtbl = &pwv2->add_script_on_document_created_handler_vtbl;
  pwv2->fullscreen_handler_vtbl.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(ICoreWebView2ContainsFullScreenElementChangedEventHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->fullscreen_handler_vtbl.AddRef = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2ContainsFullScreenElementChangedEventHandler * This)) &NoOpAddRef;
  pwv2->fullscreen_handler_vtbl.Release = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2ContainsFullScreenElementChangedEventHandler * This)) &NoOpRelease;
  pwv2->fullscreen_handler_vtbl.Invoke = &WebView2FullScreen_Invoke;
  pwv2->fullscreen_handler.lpVtbl = &pwv2->fullscreen_handler_vtbl;
  pwv2->accelerator_handler_vtbl.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(ICoreWebView2AcceleratorKeyPressedEventHandler * This, REFIID riid, void **ppvObject)) &NoOpQueryInterface;
  pwv2->accelerator_handler_vtbl.AddRef = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2AcceleratorKeyPressedEventHandler * This)) &NoOpAddRef;
  pwv2->accelerator_handler_vtbl.Release = (ULONG (STDMETHODCALLTYPE *)(ICoreWebView2AcceleratorKeyPressedEventHandler * This)) &NoOpRelease;
  pwv2->accelerator_handler_vtbl.Invoke = &WebView2AcceleratorKey_Invoke;
  pwv2->accelerator_handler.lpVtbl = &pwv2->accelerator_handler_vtbl;
}

WEBVIEW2_WIN32_API webview2 * CreateWebView2(HWND hwnd, const char *url, int debug) {
  WCHAR dirname[WEBVIEW_PATH_MAX];
  webview2 *pwv2 = NULL;
  if (CreateCoreWebView2EnvironmentFn != NULL) {
    pwv2 = (webview2 *)GlobalAlloc(GMEM_FIXED, sizeof(webview2));
    if (pwv2 != NULL) {
      /* the original code left controller/cb/context uninitialized, which
         could be read before the controller callback fired */
      ZeroMemory(pwv2, sizeof(webview2));
      InitWebView2(pwv2);
      pwv2->ready = 0;
      pwv2->jsToEval = NULL;
      pwv2->webview = NULL;
      pwv2->hwnd = hwnd;
      pwv2->url = url;
      pwv2->debug = debug;
      PCWSTR userDataFolder = getUserData(dirname, WEBVIEW_PATH_MAX);
      HRESULT hr = CreateCoreWebView2EnvironmentFn(NULL, userDataFolder, NULL, &pwv2->env_created_handler);
      if (FAILED(hr)) {
        char buffer[256];
        sprintf(buffer, CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME "(%p, %p, %p) => %08X (fail)", NULL, userDataFolder, NULL, hr);
        webview_print_log(buffer);
        GlobalFree(pwv2);
        pwv2 = NULL;
      }
    }
  } else {
    webview_print_log("CreateWebView2 error WebView2 is not enabled");
  }
  return pwv2;
}

WEBVIEW2_WIN32_API void ReleaseWebView2(webview2 *pwv2) {
  if (pwv2 != NULL) {
    if (pwv2->controller != NULL) {
      pwv2->controller->lpVtbl->Close(pwv2->controller);
      pwv2->controller->lpVtbl->Release(pwv2->controller);
      pwv2->controller = NULL;
    }
    if (pwv2->webview != NULL) {
      pwv2->webview->lpVtbl->Release(pwv2->webview);
      pwv2->webview = NULL;
    }
    if (pwv2->env != NULL) {
      pwv2->env->lpVtbl->Release(pwv2->env);
      pwv2->env = NULL;
    }
    if (pwv2->jsToEval != NULL) {
      free(pwv2->jsToEval);
      pwv2->jsToEval = NULL;
    }
    if (pwv2->htmlToSet != NULL) {
      free(pwv2->htmlToSet);
      pwv2->htmlToSet = NULL;
    }
    GlobalFree(pwv2);
  }
}

WEBVIEW2_WIN32_API void WebView2SetBounds(webview2 *pwv2, RECT bounds) {
  if ((pwv2 != NULL) && (pwv2->controller != NULL) && (pwv2->webview != NULL)) {
    pwv2->controller->lpVtbl->put_Bounds(pwv2->controller, bounds);
  }
}

WEBVIEW2_WIN32_API void WebView2RegisterCallback(webview2 *pwv2, WebView2CallbackFn cb, void *context) {
  if (pwv2 != NULL) {
    pwv2->cb = cb;
    pwv2->context = context;
  }
}

/* Set the colour WebView2 paints where the page paints nothing. Alpha 0 is
   what makes a transparent window possible; any other colour is useful for
   working out whether this call is reaching the runtime at all.

   This is ICoreWebView2Controller2::put_DefaultBackgroundColor, which arrived
   in runtime 1.0.774.44 (mid 2020). Every failure is logged with its HRESULT
   rather than passed over quietly. */
static int WebView2SetBackground(webview2 *pwv2, BYTE r, BYTE g, BYTE b,
                                 BYTE a) {
  ICoreWebView2Controller2 *controller2 = NULL;
  COREWEBVIEW2_COLOR color;
  char buffer[160];
  HRESULT hr;
  if (pwv2 == NULL) {
    return -1;
  }
  if (pwv2->controller == NULL) {
    webview_print_log("background: no controller yet, will apply later");
    return 0;
  }
  hr = pwv2->controller->lpVtbl->QueryInterface(
      pwv2->controller, iid_unref(&IID_ICoreWebView2Controller2),
      (void **)&controller2);
  if (FAILED(hr) || (controller2 == NULL)) {
    sprintf(buffer,
            "background: no ICoreWebView2Controller2 (0x%08lX) - the WebView2 "
            "runtime is older than 1.0.774.44",
            (unsigned long)hr);
    webview_print_log(buffer);
    return -1;
  }
  color.A = a;
  color.R = r;
  color.G = g;
  color.B = b;
  hr = controller2->lpVtbl->put_DefaultBackgroundColor(controller2, color);
  controller2->lpVtbl->Release(controller2);
  sprintf(buffer, "background: put_DefaultBackgroundColor(%u,%u,%u,a=%u) -> 0x%08lX",
          (unsigned)r, (unsigned)g, (unsigned)b, (unsigned)a,
          (unsigned long)hr);
  webview_print_log(buffer);
  return SUCCEEDED(hr) ? 0 : -1;
}

static int WebView2SetTransparent(webview2 *pwv2, int transparent) {
  if (pwv2 == NULL) {
    return -1;
  }
  pwv2->transparent = transparent ? 1 : 0;
  if (transparent) {
    return WebView2SetBackground(pwv2, 0, 0, 0, 0);
  }
  return WebView2SetBackground(pwv2, 255, 255, 255, 255);
}

/* Register a script that runs before every page's own scripts, on this and
   every future navigation. Used to reinstall bindings after a reload. */
WEBVIEW2_WIN32_API int WebView2AddInitScript(webview2 *pwv2, const char *js) {
  wchar_t *wjs;
  HRESULT hr;
  if ((pwv2 == NULL) || (pwv2->webview == NULL) || (js == NULL)) {
    return -1;
  }
  wjs = webview_to_utf16(js);
  if (wjs == NULL) {
    return -1;
  }
  hr = pwv2->webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(
      pwv2->webview, wjs, &pwv2->add_script_on_document_created_handler);
  GlobalFree(wjs);
  return SUCCEEDED(hr) ? 0 : -1;
}

WEBVIEW2_WIN32_API int WebView2SetHtml(webview2 *pwv2, const char *html) {
  wchar_t *whtml;
  HRESULT hr;
  if ((pwv2 == NULL) || (html == NULL)) {
    return -1;
  }
  if (pwv2->webview == NULL) {
    /* The environment, the controller and the webview are all created
       asynchronously: none of them exist yet when webview_init() returns.
       Keep the html and navigate to it in the controller callback. */
    size_t n = strlen(html) + 1;
    char *copy = (char *)malloc(n);
    if (copy == NULL) {
      return -1;
    }
    memcpy(copy, html, n);
    free(pwv2->htmlToSet);
    pwv2->htmlToSet = copy;
    return 0;
  }
  whtml = webview_to_utf16(html);
  if (whtml == NULL) {
    return -1;
  }
  hr = pwv2->webview->lpVtbl->NavigateToString(pwv2->webview, whtml);
  GlobalFree(whtml);
  return SUCCEEDED(hr) ? 0 : -1;
}

static char * formatJavaScriptForEval(const char *js) {
  static const char *prologue = "(function(){";
  static const char *epilogue = ";})();";
  int n = strlen(prologue) + strlen(epilogue) + strlen(js) + 1;
  char *fjs = (char *)malloc(n);
  if (fjs != NULL) {
    snprintf(fjs, n, "%s%s%s", prologue, js, epilogue);
  }
  return fjs;
}

WEBVIEW2_WIN32_API int WebView2Eval(webview2 *pwv2, const char *js) {
  if (pwv2->webview != NULL) {
    char *fjs = formatJavaScriptForEval(js);
    if (fjs != NULL) {
      wchar_t *wjs = webview_to_utf16(fjs);
      free(fjs);
      if (wjs != NULL) {
        ICoreWebView2ExecuteScriptCompletedHandler *handler = NULL;
        pwv2->webview->lpVtbl->ExecuteScript(pwv2->webview, wjs, handler);
        GlobalFree(wjs);
        return 0;
      }
    }
  }
  webview_print_log("WebView2Eval() not available yet");
  return 1;
}

/* This handler is allocated per call, which means - unlike the long lived
   handlers above - its vtbl lives in the same block as the object. Freeing it
   inside Invoke() left WebView2 calling Release() through a dangling vtbl
   pointer immediately afterwards. So: real reference counting, and the last
   reference is dropped from the message loop, once COM is completely done
   with the object. */

static HRESULT STDMETHODCALLTYPE EvalCtx_QueryInterface(
    ICoreWebView2ExecuteScriptCompletedHandler *This, REFIID riid,
    void **ppvObject) {
  if (ppvObject == NULL) {
    return E_POINTER;
  }
  /* The shared NoOpQueryInterface said S_OK to every interface without ever
     writing *ppvObject. Answer honestly instead. */
  if (iid_eq(riid, &IID_IUnknown) ||
      iid_eq(riid, &IID_ICoreWebView2ExecuteScriptCompletedHandler)) {
    *ppvObject = This;
    InterlockedIncrement(&((webview2_eval_ctx *)This)->refcount);
    return S_OK;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
EvalCtx_AddRef(ICoreWebView2ExecuteScriptCompletedHandler *This) {
  return (ULONG)InterlockedIncrement(&((webview2_eval_ctx *)This)->refcount);
}

static ULONG STDMETHODCALLTYPE
EvalCtx_Release(ICoreWebView2ExecuteScriptCompletedHandler *This) {
  webview2_eval_ctx *ctx = (webview2_eval_ctx *)This;
  LONG n = InterlockedDecrement(&ctx->refcount);
  if (n <= 0) {
    free(ctx);
    return 0;
  }
  return (ULONG)n;
}

/* Runs from the message loop, after Invoke() has returned and WebView2 has
   let go of the handler. */
static void WebView2EvalReleaseDispatched(struct webview *w, void *arg) {
  webview2_eval_ctx *ctx = (webview2_eval_ctx *)arg;
  ctx->handler.lpVtbl->Release(&ctx->handler);
}

static HRESULT STDMETHODCALLTYPE ExecuteScriptCompleted_Invoke(
    ICoreWebView2ExecuteScriptCompletedHandler *This, HRESULT errorCode,
    LPCWSTR resultObjectAsJson) {
  webview2_eval_ctx *ctx = (webview2_eval_ctx *)This;
  char *result = NULL;
  if (SUCCEEDED(errorCode) && (resultObjectAsJson != NULL)) {
    result = webview_from_utf16((WCHAR *)resultObjectAsJson);
  }
  if (FAILED(errorCode)) {
    char buffer[128];
    sprintf(buffer, "ExecuteScript failed with 0x%08lX",
            (unsigned long)errorCode);
    webview_print_log(buffer);
  }
  if (ctx->cb != NULL) {
    ctx->cb(ctx->userdata, SUCCEEDED(errorCode) ? 0 : 1,
            result != NULL ? result : "null");
  }
  if (result != NULL) {
    GlobalFree(result);
  }
  /* do NOT free here */
  PostMessageW(ctx->hwnd, WM_WEBVIEW_DISPATCH,
               (WPARAM)&WebView2EvalReleaseDispatched, (LPARAM)ctx);
  return S_OK;
}

/* ExecuteScript on any ICoreWebView2 - the main one or a <webview> guest.
   `hwnd` is the top level window whose message loop drops the last
   reference. `cb` may be NULL. */
static int WebView2ExecuteOn(ICoreWebView2 *view, HWND hwnd, const char *js,
                             WebView2EvalCallbackFn cb, void *userdata) {
  webview2_eval_ctx *ctx;
  wchar_t *wjs;
  HRESULT hr;
  if ((view == NULL) || (js == NULL)) {
    return -1;
  }
  ctx = (webview2_eval_ctx *)malloc(sizeof(webview2_eval_ctx));
  if (ctx == NULL) {
    return -1;
  }
  ctx->vtbl.QueryInterface = &EvalCtx_QueryInterface;
  ctx->vtbl.AddRef = &EvalCtx_AddRef;
  ctx->vtbl.Release = &EvalCtx_Release;
  ctx->vtbl.Invoke = &ExecuteScriptCompleted_Invoke;
  ctx->handler.lpVtbl = &ctx->vtbl;
  ctx->cb = cb;
  ctx->userdata = userdata;
  ctx->hwnd = hwnd;
  ctx->refcount = 1;
  wjs = webview_to_utf16(js);
  if (wjs == NULL) {
    free(ctx);
    return -1;
  }
  hr = view->lpVtbl->ExecuteScript(view, wjs, &ctx->handler);
  GlobalFree(wjs);
  if (FAILED(hr)) {
    free(ctx);
    return -1;
  }
  return 0;
}

WEBVIEW2_WIN32_API int WebView2EvalResult(webview2 *pwv2, const char *js,
                                          WebView2EvalCallbackFn cb,
                                          void *userdata) {
  if ((pwv2 == NULL) || (pwv2->webview == NULL)) {
    webview_print_log("WebView2EvalResult() not available yet");
    return -1;
  }
  return WebView2ExecuteOn(pwv2->webview, pwv2->hwnd, js, cb, userdata);
}

static int WebView2EnableVersion(GetWebView2BrowserVersionInfoFnType getVersionInfoFn, CreateCoreWebView2EnvironmentWithOptionsFnType createEnvWithOptionsFn) {
  if ((createEnvWithOptionsFn != NULL) && (getVersionInfoFn != NULL)) {
    LPWSTR versionInfo = NULL;
    HRESULT hr = getVersionInfoFn(NULL, &versionInfo);
    if ((hr == S_OK) && (versionInfo != NULL)) {
      char *version = webview_from_utf16(versionInfo);
      CreateCoreWebView2EnvironmentFn = createEnvWithOptionsFn;
      webview2_setDiag("WebView2 runtime %s",
                       version != NULL ? version : "(unknown version)");
      if (version != NULL) {
        GlobalFree(version);
      }
      CoTaskMemFree(versionInfo);
      return 1;
    } else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
      webview2_setDiag("the WebView2 runtime is not installed on this machine "
                       "(install the Evergreen runtime, " WEBVIEW2_ARCH_DIR
                       " or newer Windows)");
    } else {
      webview2_setDiag("the WebView2 runtime could not be detected (0x%08lX)",
                       (unsigned long)hr);
    }
  } else {
    webview2_setDiag(WEBVIEW2_LOADER_DLL
                     " is missing the expected entry points, it is probably "
                     "an old or corrupted copy");
  }
  return 0;
}

#if defined(WEBVIEW2_MEMORY_MODULE)
#include "MemoryModule.h"
#ifndef WEBVIEW2_LOADER_RES_NAME
#define WEBVIEW2_LOADER_RES_NAME "WEBVIEW2LOADER"
#endif
/* Map the embedded WebView2Loader.dll straight out of our own image: no file
   is ever written, no LoadLibrary is ever called on it. MemoryLoadLibrary
   does what the Windows loader would have done - copy the sections, apply
   relocations, resolve the imports, run DllMain. */
static int webview2_enableFromMemory(void) {
  const void *data;
  DWORD size = 0;
  HMEMORYMODULE module;
  GetWebView2BrowserVersionInfoFnType getVersionInfoFn;
  CreateCoreWebView2EnvironmentWithOptionsFnType createEnvWithOptionsFn;
  /* the bytes come from the resource or the array - see wv_loader.inl */
  data = webview2_loaderBytes(&size);
  if ((data == NULL) || (size == 0)) {
    return 0;
  }
  module = MemoryLoadLibrary(data, (size_t)size);
  if (module == NULL) {
    webview2_setDiag("in-memory load of " WEBVIEW2_LOADER_DLL
                     " failed (error %lu), falling back to the file",
                     (unsigned long)GetLastError());
    return 0;
  }
  getVersionInfoFn = (GetWebView2BrowserVersionInfoFnType)MemoryGetProcAddress(
      module, GET_AVAILABLE_COREWEBVIEW2_BROWSER_VERSION_FN_NAME);
  createEnvWithOptionsFn =
      (CreateCoreWebView2EnvironmentWithOptionsFnType)MemoryGetProcAddress(
          module, CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME);
  if (WebView2EnableVersion(getVersionInfoFn, createEnvWithOptionsFn)) {
    webview_print_log(WEBVIEW2_LOADER_DLL " mapped from memory, nothing on disk");
    return 1;
  }
  MemoryFreeLibrary(module);
  return 0;
}
#endif /* WEBVIEW2_MEMORY_MODULE */

static int WebView2Enable() {
  webview_print_log("Loading WebView2Loader");
  if (CreateCoreWebView2EnvironmentFn != NULL) {
    return 1;
  }
  if (getenv("WEBVIEW_DISABLE_WEBVIEW2") != NULL) {
    webview2_setDiag("WebView2 disabled by WEBVIEW_DISABLE_WEBVIEW2");
    return 0;
  }
#if defined(CORE_WEBVIEW_TARGET_PRODUCT_VERSION)
  OutputDebugStringW(CORE_WEBVIEW_TARGET_PRODUCT_VERSION);
#endif
  findWebView2BrowserExecutableFolder();
#if defined(WEBVIEW2_MEMORY_MODULE)
  /* preferred: never touch the file system at all */
  if (webview2_enableFromMemory()) {
    return 1;
  }
#endif
  HMODULE hWebView2LoaderModule = webview2_findLoader();
  if (hWebView2LoaderModule == NULL) {
    webview2_setDiag(WEBVIEW2_LOADER_DLL " (" WEBVIEW2_ARCH_DIR
                     ") was not found next to the executable, in a "
                     WEBVIEW2_ARCH_DIR " subfolder, or on the PATH - ship it "
                     "with your exe or set " WEBVIEW2_WIN32_PATH);
    return 0;
  }
  GetWebView2BrowserVersionInfoFnType getVersionInfoFn = (GetWebView2BrowserVersionInfoFnType)GetProcAddress(hWebView2LoaderModule, GET_AVAILABLE_COREWEBVIEW2_BROWSER_VERSION_FN_NAME);
  CreateCoreWebView2EnvironmentWithOptionsFnType createEnvWithOptionsFn = (CreateCoreWebView2EnvironmentWithOptionsFnType)GetProcAddress(hWebView2LoaderModule, CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME);
  return WebView2EnableVersion(getVersionInfoFn, createEnvWithOptionsFn);
}

#if defined(WEBVIEW2_MEMORY_MODULE)
#include "MemoryModule.h"
WEBVIEW_API int WebView2Load(const void *data, size_t size) {
  if (CreateCoreWebView2EnvironmentFn != NULL) {
    return 1;
  }
  HMEMORYMODULE hWebView2LoaderModule = MemoryLoadLibrary(data, size);
  if (hWebView2LoaderModule == NULL) {
    webview_print_log("Unable to load");
    return 0;
  }
  //  void MemoryFreeLibrary(HMEMORYMODULE);
  GetWebView2BrowserVersionInfoFnType getVersionInfoFn = (GetWebView2BrowserVersionInfoFnType)MemoryGetProcAddress(hWebView2LoaderModule, GET_AVAILABLE_COREWEBVIEW2_BROWSER_VERSION_FN_NAME);
  CreateCoreWebView2EnvironmentWithOptionsFnType createEnvWithOptionsFn = (CreateCoreWebView2EnvironmentWithOptionsFnType)MemoryGetProcAddress(hWebView2LoaderModule, CREATE_COREWEBVIEW2_ENVIRONMENTWITHOPTIONS_FN_NAME);
  return WebView2EnableVersion(getVersionInfoFn, createEnvWithOptionsFn);
}
#endif
