/* ---------------------------------------------------------------------------
 * wv_api.inl - the public messaging API
 *
 * webview_eval, webview_dispatch, and the C -> JavaScript event API
 * (webview_emit / emit_str / emit_async / eval_result).
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

WEBVIEW_API int webview_eval(struct webview *w, const char *js) {
  if (webview_webview2_enabled) {
    webview2 *pwv2 = w->priv.webview2;
    if (pwv2 == NULL) {
      webview_print_log("webview_eval() called before the window exists");
      return -1;
    }
    if (pwv2->ready) {
      return WebView2Eval(pwv2, js);
    }
    char *jsToEval = pwv2->jsToEval;
    webview_print_log("defer eval as webview2 not ready");
    if (jsToEval == NULL) {
      int n = strlen(js) + 1;
      char *newJsToEval = (char *)malloc(n);
      strncpy(newJsToEval, js, n);
      pwv2->jsToEval = newJsToEval;
    } else {
      int n = strlen(jsToEval) + 1 + strlen(js) + 1;
      char *newJsToEval = (char *)malloc(n);
      snprintf(newJsToEval, n, "%s\n%s", jsToEval, js);
      free(jsToEval);
      pwv2->jsToEval = newJsToEval;
    }
    return 0;
  }
  IWebBrowser2 *webBrowser2;
  IHTMLDocument2 *htmlDoc2;
  IDispatch *docDispatch;
  IDispatch *scriptDispatch;
  if ((w->priv.browser == NULL) || (*w->priv.browser == NULL)) {
    /* no window yet (called before webview_init) - nothing to run it in */
    webview_print_log("webview_eval() called before the window exists");
    return -1;
  }
  if ((*w->priv.browser)
          ->lpVtbl->QueryInterface((*w->priv.browser),
                                   iid_unref(&IID_IWebBrowser2),
                                   (void **)&webBrowser2) != S_OK) {
    return -1;
  }

  if (webBrowser2->lpVtbl->get_Document(webBrowser2, &docDispatch) != S_OK) {
    return -1;
  }
  if (docDispatch->lpVtbl->QueryInterface(docDispatch,
                                          iid_unref(&IID_IHTMLDocument2),
                                          (void **)&htmlDoc2) != S_OK) {
    return -1;
  }
  if (htmlDoc2->lpVtbl->get_Script(htmlDoc2, &scriptDispatch) != S_OK) {
    return -1;
  }
  DISPID dispid;
  wchar_t *evalStr = L"eval";
  if (scriptDispatch->lpVtbl->GetIDsOfNames(
          scriptDispatch, iid_unref(&IID_NULL), &evalStr, 1,
          LOCALE_SYSTEM_DEFAULT, &dispid) != S_OK) {
    return -1;
  }

  DISPPARAMS params;
  VARIANT arg;
  VARIANT result;
  EXCEPINFO excepInfo;
  UINT nArgErr = (UINT)-1;
  params.cArgs = 1;
  params.cNamedArgs = 0;
  params.rgvarg = &arg;
  arg.vt = VT_BSTR;
  static const char *prologue = "(function(){";
  static const char *epilogue = ";})();";
  int n = strlen(prologue) + strlen(epilogue) + strlen(js) + 1;
  char *eval = (char *)malloc(n);
  snprintf(eval, n, "%s%s%s", prologue, js, epilogue);
  {
    /* modern code for an old engine: translated, with its runtime */
    char *legacy = webview_legacy_script(eval, strlen(eval), "webview_eval");
    if (legacy != NULL) {
      webview_mshtml_ensure_runtime(scriptDispatch);
      free(eval);
      eval = legacy;
    }
  }
  wchar_t *buf = webview_to_utf16(eval);
  if (buf == NULL) {
    return -1;
  }
  arg.bstrVal = SysAllocString(buf);
  if (scriptDispatch->lpVtbl->Invoke(
          scriptDispatch, dispid, iid_unref(&IID_NULL), 0, DISPATCH_METHOD,
          &params, &result, &excepInfo, &nArgErr) != S_OK) {
    return -1;
  }
  SysFreeString(arg.bstrVal);
  free(eval);
  scriptDispatch->lpVtbl->Release(scriptDispatch);
  htmlDoc2->lpVtbl->Release(htmlDoc2);
  docDispatch->lpVtbl->Release(docDispatch);
  return 0;
}

/* Show a string of HTML instead of navigating to a URL. Call it after
   webview_init(). On WebView2 this is ICoreWebView2::NavigateToString; on the
   MSHTML fallback the html is handed over as a data: url, which means a
   literal "%" followed by two hex digits would be decoded - escape it as %25
   if you target Windows 7. */
WEBVIEW_API int webview_set_html(struct webview *w, const char *html) {
  char *url;
  size_t n;
  int r;
  if ((w == NULL) || (html == NULL)) {
    return -1;
  }
  if (webview_webview2_enabled) {
    /* Safe to call straight after webview_init(): if the webview has not
       been created yet the html is remembered and used as the first
       navigation. */
    return WebView2SetHtml(w->priv.webview2, html);
  }
  n = strlen(WEBVIEW_DATA_URL_PREFIX) + strlen(html) + 1;
  url = (char *)malloc(n);
  if (url == NULL) {
    return -1;
  }
  snprintf(url, n, "%s%s", WEBVIEW_DATA_URL_PREFIX, html);
  w->url = url; /* DisplayHTMLPage reads w->url */
  r = DisplayHTMLPage(w);
  return r;
}

WEBVIEW_API void webview_dispatch(struct webview *w, webview_dispatch_fn fn,
                                  void *arg) {
  PostMessageW(w->priv.hwnd, WM_WEBVIEW_DISPATCH, (WPARAM)fn, (LPARAM)arg);
}

/* ------------------------------------------------------------------------
 * Bound functions
 *
 * Wire format, built by api.__call in the bootstrap:
 *
 *     __wv:<id>:<name length>:<name><arguments as a JSON array>
 *
 * The name is length prefixed so nothing has to be escaped and no JSON
 * parser is needed here - the arguments are handed to your callback as the
 * raw JSON text that follows.
 * ------------------------------------------------------------------------ */

static char *webview_strdup(const char *s) {
  size_t n;
  char *copy;
  if (s == NULL) {
    return NULL;
  }
  n = strlen(s) + 1;
  copy = (char *)malloc(n);
  if (copy != NULL) {
    memcpy(copy, s, n);
  }
  return copy;
}

/* window.webview.__bind("name") - creates window.name (dots make namespaces) */
static void webview_install_one_binding(struct webview *w, const char *name) {
  char *escaped = webview_json_escape(name);
  char *js;
  size_t n;
  if (escaped == NULL) {
    return;
  }
  n = strlen(escaped) + 64;
  js = (char *)malloc(n);
  if (js != NULL) {
    snprintf(js, n, "window.webview.__bind(%s);", escaped);
    /* for the page that is open right now... (the IE control has no
       init-script hook, so there the bus has to come along, like emit does) */
    if (!webview_webview2_enabled) {
      char *withbus = (char *)malloc(n + sizeof(WEBVIEW_JS_BOOTSTRAP) + 8);
      if (withbus != NULL) {
        sprintf(withbus, "%s%s", WEBVIEW_JS_BOOTSTRAP, js);
        webview_eval(w, withbus);
        free(withbus);
      }
    } else {
      webview_eval(w, js);
    }
    /* ...and for every page loaded afterwards */
    if (webview_webview2_enabled && (w->priv.webview2 != NULL)) {
      char *init = (char *)malloc(n + sizeof(WEBVIEW_JS_BOOTSTRAP) + 8);
      if (init != NULL) {
        sprintf(init, "%s%s", WEBVIEW_JS_BOOTSTRAP, js);
        WebView2AddInitScript(w->priv.webview2, init);
        free(init);
      }
    }
    free(js);
  }
  free(escaped);
}

WEBVIEW_API void webview_install_bindings(struct webview *w) {
  int i;
  if (w == NULL) {
    return;
  }
  for (i = 0; i < w->priv.binding_count; i++) {
    webview_install_one_binding(w, w->priv.bindings[i].name);
  }
}

WEBVIEW_API int webview_bind(struct webview *w, const char *name,
                             webview_bind_fn fn, void *userdata) {
  struct webview_binding *slot;
  int i;
  if ((w == NULL) || (name == NULL) || (fn == NULL)) {
    return -1;
  }
  /* rebinding a name replaces it */
  for (i = 0; i < w->priv.binding_count; i++) {
    if (strcmp(w->priv.bindings[i].name, name) == 0) {
      w->priv.bindings[i].fn = fn;
      w->priv.bindings[i].userdata = userdata;
      return 0;
    }
  }
  if (w->priv.binding_count == w->priv.binding_capacity) {
    int cap = (w->priv.binding_capacity == 0) ? 8 : w->priv.binding_capacity * 2;
    struct webview_binding *grown = (struct webview_binding *)realloc(
        w->priv.bindings, sizeof(struct webview_binding) * (size_t)cap);
    if (grown == NULL) {
      return -1;
    }
    w->priv.bindings = grown;
    w->priv.binding_capacity = cap;
  }
  slot = &w->priv.bindings[w->priv.binding_count];
  slot->name = webview_strdup(name);
  if (slot->name == NULL) {
    return -1;
  }
  slot->fn = fn;
  slot->userdata = userdata;
  w->priv.binding_count++;

  /* If the browser is not up yet this does nothing useful, but the controller
     callback calls webview_install_bindings() once it is. */
  webview_install_one_binding(w, name);
  return 0;
}

WEBVIEW_API int webview_unbind(struct webview *w, const char *name) {
  int i;
  if ((w == NULL) || (name == NULL)) {
    return -1;
  }
  for (i = 0; i < w->priv.binding_count; i++) {
    if (strcmp(w->priv.bindings[i].name, name) == 0) {
      char *escaped = webview_json_escape(name);
      char *js;
      size_t n;
      if (escaped != NULL) {
        n = strlen(escaped) + 64;
        js = (char *)malloc(n);
        if (js != NULL) {
          snprintf(js, n, "window.webview.__unbind(%s);", escaped);
          webview_eval(w, js);
          free(js);
        }
        free(escaped);
      }
      free(w->priv.bindings[i].name);
      w->priv.bindings[i] = w->priv.bindings[w->priv.binding_count - 1];
      w->priv.binding_count--;
      return 0;
    }
  }
  return -1;
}

/* Settle the promise on the JS side. Safe from any thread. */
WEBVIEW_API int webview_return(struct webview *w, const char *seq, int status,
                               const char *result_json) {
  char *js;
  size_t n;
  int r;
  if ((w == NULL) || (seq == NULL)) {
    return -1;
  }
  if (result_json == NULL) {
    result_json = "null";
  }
  n = strlen(seq) + strlen(result_json) + 96;
  js = (char *)malloc(n);
  if (js == NULL) {
    return -1;
  }
  if ((size_t)snprintf(js, n,
                       "window.webview.__resolve(%s,%d,%s);", seq, status,
                       result_json) >= n) {
    free(js);
    return -1;
  }
  if (GetCurrentThreadId() == GetWindowThreadProcessId(w->priv.hwnd, NULL)) {
    r = webview_eval(w, js);
    free(js);
  } else {
    /* another thread: hand the script to the UI thread, which frees it */
    webview_dispatch(w, &webview_eval_dispatched, js);
    r = 0;
  }
  return r;
}

/* Returns 1 when the message was one of ours and has been handled. */
static int webview_dispatch_binding(struct webview *w, const char *message) {
  const char *p;
  const char *seq_start;
  char seq[24];
  size_t seq_len;
  int name_len;
  int i;
  if ((w == NULL) || (message == NULL) ||
      (strncmp(message, "__wv:", 5) != 0)) {
    return 0;
  }
  p = message + 5;
  seq_start = p;
  while ((*p >= '0') && (*p <= '9')) {
    p++;
  }
  seq_len = (size_t)(p - seq_start);
  if ((seq_len == 0) || (seq_len >= sizeof(seq)) || (*p != ':')) {
    return 0;
  }
  memcpy(seq, seq_start, seq_len);
  seq[seq_len] = '\0';
  p++;
  name_len = 0;
  while ((*p >= '0') && (*p <= '9')) {
    name_len = name_len * 10 + (*p - '0');
    p++;
  }
  if ((name_len <= 0) || (*p != ':')) {
    return 0;
  }
  p++;
  if ((int)strlen(p) < name_len) {
    return 0;
  }
  for (i = 0; i < w->priv.binding_count; i++) {
    const char *name = w->priv.bindings[i].name;
    if (((int)strlen(name) == name_len) &&
        (strncmp(p, name, (size_t)name_len) == 0)) {
      w->priv.bindings[i].fn(w, seq, p + name_len,
                             w->priv.bindings[i].userdata);
      return 1;
    }
  }
  /* a name the page knows about but C no longer does */
  webview_return(w, seq, 1, "\"no such binding\"");
  return 1;
}

/* ------------------------------------------------------------------------
 * C -> JavaScript
 * ------------------------------------------------------------------------ */

WEBVIEW_API int webview_emit(struct webview *w, const char *name,
                             const char *json) {
  char *escapedName;
  const char *payload;
  const char *prefix;
  size_t size;
  char *js;
  int r;
  if ((w == NULL) || (name == NULL)) {
    return -1;
  }
  escapedName = webview_json_escape(name);
  if (escapedName == NULL) {
    return -1;
  }
  payload = ((json == NULL) || (json[0] == '\0')) ? "null" : json;
  /* WebView2 installs the bus with AddScriptToExecuteOnDocumentCreated;
     the IE control has no such hook, so carry it along (it is idempotent). */
  prefix = webview_webview2_enabled ? "" : WEBVIEW_JS_BOOTSTRAP;
  /* The fixed part of the format string below is 68 characters. 64 bytes of
     slack silently truncated the tail of every single event. */
  size = strlen(prefix) + strlen(escapedName) + strlen(payload) + 128;
  js = (char *)malloc(size);
  if (js == NULL) {
    free(escapedName);
    return -1;
  }
  if ((size_t)snprintf(js, size,
                       "%sif(window.webview&&window.webview.receive)"
                       "window.webview.receive(%s,%s);",
                       prefix, escapedName, payload) >= size) {
    webview_print_log("webview_emit: script buffer too small, event dropped");
    free(js);
    free(escapedName);
    return -1;
  }
  r = webview_eval(w, js);
  free(js);
  free(escapedName);
  return r;
}

WEBVIEW_API int webview_emit_str(struct webview *w, const char *name,
                                 const char *value) {
  char *escaped = webview_json_escape(value);
  int r;
  if (escaped == NULL) {
    return -1;
  }
  r = webview_emit(w, name, escaped);
  free(escaped);
  return r;
}

struct webview_emit_msg {
  char *name;
  char *json;
};

/* runs on the UI thread, frees the script it was given */
static void webview_eval_dispatched(struct webview *w, void *arg) {
  char *js = (char *)arg;
  if (js != NULL) {
    webview_eval(w, js);
    free(js);
  }
}

static void webview_emit_dispatched(struct webview *w, void *arg) {
  struct webview_emit_msg *msg = (struct webview_emit_msg *)arg;
  if (msg == NULL) {
    return;
  }
  webview_emit(w, msg->name, msg->json);
  free(msg->name);
  free(msg->json);
  free(msg);
}

WEBVIEW_API int webview_emit_async(struct webview *w, const char *name,
                                   const char *json) {
  struct webview_emit_msg *msg;
  size_t n;
  if ((w == NULL) || (name == NULL)) {
    return -1;
  }
  msg = (struct webview_emit_msg *)calloc(1, sizeof(struct webview_emit_msg));
  if (msg == NULL) {
    return -1;
  }
  n = strlen(name) + 1;
  msg->name = (char *)malloc(n);
  if (msg->name == NULL) {
    free(msg);
    return -1;
  }
  memcpy(msg->name, name, n);
  if (json != NULL) {
    n = strlen(json) + 1;
    msg->json = (char *)malloc(n);
    if (msg->json == NULL) {
      free(msg->name);
      free(msg);
      return -1;
    }
    memcpy(msg->json, json, n);
  }
  webview_dispatch(w, &webview_emit_dispatched, msg);
  return 0;
}

struct webview_eval_result_ctx {
  struct webview *w;
  webview_eval_cb_t cb;
  void *userdata;
};

static void webview_eval_result_trampoline(void *userdata, int failed,
                                           const char *json) {
  struct webview_eval_result_ctx *ctx =
      (struct webview_eval_result_ctx *)userdata;
  if (ctx == NULL) {
    return;
  }
  if (ctx->cb != NULL) {
    ctx->cb(ctx->w, failed ? "null" : json, ctx->userdata);
  }
  free(ctx);
}

WEBVIEW_API int webview_eval_result(struct webview *w, const char *js,
                                    webview_eval_cb_t cb, void *userdata) {
  struct webview_eval_result_ctx *ctx;
  int r;
  if ((w == NULL) || (js == NULL)) {
    return -1;
  }
  if (!webview_webview2_enabled || (w->priv.webview2 == NULL)) {
    /* MSHTML/IE fallback: no result plumbing, use window.external.invoke
       from the page instead. */
    webview_print_log("webview_eval_result() requires WebView2");
    return -1;
  }
  ctx = (struct webview_eval_result_ctx *)malloc(
      sizeof(struct webview_eval_result_ctx));
  if (ctx == NULL) {
    return -1;
  }
  ctx->w = w;
  ctx->cb = cb;
  ctx->userdata = userdata;
  r = WebView2EvalResult(w->priv.webview2, js,
                         &webview_eval_result_trampoline, ctx);
  if (r != 0) {
    free(ctx);
  }
  return r;
}

WEBVIEW_API int webview_inject_css(struct webview *w, const char *css) {
  int n = webview_js_encode(css, NULL, 0);
  char *esc = (char *)calloc(1, sizeof(CSS_INJECT_FUNCTION) + n + 4);
  if (esc == NULL) {
    return -1;
  }
  char *js = (char *)calloc(1, n);
  webview_js_encode(css, js, n);
  snprintf(esc, sizeof(CSS_INJECT_FUNCTION) + n + 4, "%s(\"%s\")",
           CSS_INJECT_FUNCTION, js);
  int r = webview_eval(w, esc);
  free(js);
  free(esc);
  return r;
}

WEBVIEW_API int webview_run(const char *title, const char *url, int width,
                        int height, int resizable) {
  struct webview webview;
  memset(&webview, 0, sizeof(webview));
  webview.title = title;
  webview.url = url;
  webview.width = width;
  webview.height = height;
  webview.resizable = resizable;
  int r = webview_init(&webview);
  if (r != 0) {
    return r;
  }
  while (webview_loop(&webview, 1) == 0) {
  }
  webview_exit(&webview);
  return 0;
}
