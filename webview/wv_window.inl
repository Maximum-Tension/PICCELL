/* ---------------------------------------------------------------------------
 * wv_window.inl - the native window
 *
 * Window class and icon, creation, the message loop, title/fullscreen/
 * colour, and the window procedure that glues the two backends together.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

static void WebView2Callback(webview2 *wv, const char *message, void *context) {
  struct webview *w = (struct webview *)context;
  if (w == NULL) {
    return;
  }
  /* the page telling us it is up: apply the w.window options that need the
     browser to exist, then swallow it - the user never sees this message */
  if (strcmp(message, "__wvready") == 0) {
    webview_apply_page_options(w);
    return;
  }
  /* a <webview> tag talking to C */
  if (webview_guest_on_host_message(w, message)) {
    return;
  }
  /* a page that reads backdrop frames from shared memory (wv_backdrop.inl) */
  if (strcmp(message, "__wvbd shared") == 0) {
    webview_step("backdrop: the page takes shared buffers");
    webview_backdrop_page_takes_shared();
    return;
  }
  /* a call to a bound function is handled here and goes no further */
  if (webview_dispatch_binding(w, message)) {
    return;
  }
  if (w->external_invoke_cb != NULL) {
    w->external_invoke_cb(w, message);
  }
}

/* A frameless glass window (frame kept, hidden by WM_NCCALCSIZE) that also
   has a window region - a custom corner radius. Windows draws no modern
   frame for a window with a region and falls back to painting the classic
   one itself on activation, which is what Alt+Tab used to show. */
static int webview_frame_hidden_and_shaped(struct webview *w) {
  HRGN region;
  int shaped;
  if ((w == NULL) || (w->priv.hwnd == NULL) || !w->priv.hidden_frame) {
    return 0;
  }
  /* GetWindowRgn rather than GetWindowRgnBox: that one is XP and newer */
  region = CreateRectRgn(0, 0, 0, 0);
  if (region == NULL) {
    return 0;
  }
  shaped = (GetWindowRgn(w->priv.hwnd, region) != ERROR);
  DeleteObject(region);
  return shaped;
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                LPARAM lParam) {
  struct webview *w = (struct webview *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  RECT rect;
  switch (uMsg) {
  case WM_ACTIVATE:
  case WM_NCACTIVATE:
  case WM_DPICHANGED:
    /* Symptom this is aimed at: the window starts out transparent and turns
       white the moment you hover or click it. That looks like the browser
       reinitialising its surface on activation and dropping the transparent
       default background, so we put it back. Harmless if it was never lost. */
    if ((w != NULL) && w->priv.transparent && webview_webview2_enabled &&
        (w->priv.webview2 != NULL)) {
      WebView2SetTransparent(w->priv.webview2, 1);
    }
    if (uMsg == WM_ACTIVATE) {
      /* own glass: a faster backdrop while focused, see wv_backdrop.inl */
      webview_backdrop_activated(w, LOWORD(wParam) != WA_INACTIVE);
    }
    if (uMsg == WM_NCACTIVATE) {
      /* Glass: DWM flattens the material on an inactive window. Paint as
         active regardless; focus itself still moves. See wv_glass.inl. */
      WPARAM active = webview_glass_keep_active_now(w) ? TRUE : wParam;
      if (webview_frame_hidden_and_shaped(w)) {
        /* lParam -1: "do not repaint the non-client area". A window with a
           region gets no modern frame, so DefWindowProc would paint a
           classic title bar itself - straight over the page. */
        return DefWindowProc(hwnd, uMsg, active, -1);
      }
      if (active != wParam) {
        return DefWindowProc(hwnd, uMsg, active, lParam);
      }
    }
    break;
  case WM_NCPAINT:
  case 0x00AE: /* WM_NCUAHDRAWCAPTION - the theme drawing a caption */
  case 0x00AF: /* WM_NCUAHDRAWFRAME - ...and a frame */
    /* Hidden frame with a region: there is no frame to paint, and anything
       painted lands on top of the page. */
    if (webview_frame_hidden_and_shaped(w)) {
      return 0;
    }
    break;
  case WM_SETTEXT:
  case WM_SETICON:
    /* DefWindowProc repaints the caption right here, without WM_NCPAINT.
       Hide the window from that for the duration, the way Chromium does for
       its own custom frame: no visible style, nothing gets painted. */
    if (webview_frame_hidden_and_shaped(w)) {
      LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
      LRESULT result;
      SetWindowLongPtr(hwnd, GWL_STYLE, style & ~(LONG_PTR)WS_VISIBLE);
      result = DefWindowProc(hwnd, uMsg, wParam, lParam);
      SetWindowLongPtr(hwnd, GWL_STYLE, style);
      return result;
    }
    break;
  case WM_NCCALCSIZE: {
    /* Frameless glass: the whole window is client area. */
    LRESULT handled;
    if (webview_glass_nccalcsize(w, hwnd, wParam, lParam, &handled)) {
      return handled;
    }
    break;
  }
  case WM_TIMER:
    if (wParam == WEBVIEW_ERASE_TIMER) {
      /* the frame transition has finished repainting */
      webview_erase_armed = 0;
      KillTimer(hwnd, WEBVIEW_ERASE_TIMER);
      return 0;
    }
    if (wParam == WEBVIEW_GUEST_DRAG_TIMER) {
      if (w != NULL) {
        webview_guest_drag_tick(w);
      }
      return 0;
    }
    if (wParam == WEBVIEW_BACKDROP_TIMER) {
      webview_backdrop_tick(w);
      return 0;
    }
    if (wParam == WEBVIEW_BACKDROP_REVEAL_TIMER) {
      webview_backdrop_rehide(w); /* the screenshot is long taken */
      return 0;
    }
    if (wParam == WEBVIEW_TRANSPARENCY_TIMER) {
      if (!webview_retry_transparency(w)) {
        KillTimer(hwnd, WEBVIEW_TRANSPARENCY_TIMER);
      }
      return 0;
    }
    break;
  case WEBVIEW_BACKDROP_FRAME:
    /* the display refreshed: maybe a backdrop capture (wv_backdrop.inl) */
    webview_backdrop_frame(w);
    return 0;
  case WM_ENTERSIZEMOVE:
    /* about to be dragged - start erasing so old pixels cannot pile up */
    webview_erase_armed = 1;
    webview_backdrop_sizemove(w, 1);
    break;
  case WM_EXITSIZEMOVE:
    webview_erase_armed = 0;
    webview_backdrop_sizemove(w, 0);
    break;
  case WM_DWMCOMPOSITIONCHANGED:
    /* DWM restarted, or the user toggled composition. Everything we asked it
       for is gone, including the alpha channel. */
    webview_refresh_transparency(w);
    break;
  case WM_SETTINGCHANGE:
  case WM_POWERBROADCAST:
    /* Glass: transparency effects or energy saver may just have changed. */
    webview_glass_settings_changed(w, uMsg, lParam);
    break;
  case WM_ERASEBKGND:
    /* THE missing piece for transparency. Saying "I erased it" and painting
       nothing is what keeps a solid background from being filled in behind
       the browser. Without this the window comes up white however much alpha
       you gave it. */
    if ((w != NULL) && w->priv.transparent) {
      /* Paint black rather than nothing. Zero alpha reads as transparent to
         DWM, and unlike painting nothing it clears whatever the last size of
         the window left behind - see webview_erase_transparent. */
      webview_erase_transparent(w, (HDC)wParam);
      return 1;
    }
    break;
  case WM_CREATE:
    w = (struct webview *)((CREATESTRUCT *)lParam)->lpCreateParams;
    w->priv.hwnd = hwnd;
    if (webview_webview2_enabled) {
      w->priv.webview2 = CreateWebView2(hwnd, w->url, w->debug);
      if (w->priv.webview2 != NULL) {
        WebView2RegisterCallback(w->priv.webview2, &WebView2Callback, w);
      }
      return TRUE;
    }
    return EmbedBrowserObject(w);
  case WM_MOVE:
    if ((w != NULL) && webview_webview2_enabled) {
      webview_guests_parent_moved(w);
    }
    webview_backdrop_moved(w); /* what is behind the window just changed */
    break;
  case WM_DESTROY:
    webview_backdrop_destroyed(w);
    if (webview_webview2_enabled) {
      webview_guests_close_all(w);
      ReleaseWebView2(w->priv.webview2);
      w->priv.webview2 = NULL;
    } else {
      UnEmbedBrowserObject(w);
    }
    PostQuitMessage(0);
    return TRUE;
  case WM_SIZE:
    GetClientRect(hwnd, &rect);
    /* a custom corner radius is a window region, which has to follow the
       size - and go away while maximized */
    if ((w != NULL) && (w->window.corner_radius != 0)) {
      webview_apply_corners(w);
    }
    webview_backdrop_moved(w);
    if ((w != NULL) && w->priv.transparent) {
      /* The browser child lags a frame behind the resize, and the non client
         area is not repainted on its own. Without this the uncovered strip
         keeps the old border pixels and they pile up as you drag. */
      RedrawWindow(hwnd, NULL, NULL,
                   RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    }
    if (webview_webview2_enabled) {
      WebView2SetBounds(w->priv.webview2, rect);
      /* a <webview> guest in fullscreen covers the whole client area */
      webview_guests_parent_resized(w);
    } else {
      IWebBrowser2 *webBrowser2;
      IOleObject *browser = *w->priv.browser;
      if (browser->lpVtbl->QueryInterface(browser, iid_unref(&IID_IWebBrowser2),
                                          (void **)&webBrowser2) == S_OK) {
        webBrowser2->lpVtbl->put_Width(webBrowser2, rect.right);
        webBrowser2->lpVtbl->put_Height(webBrowser2, rect.bottom);
      }
    }
    return TRUE;
  case WM_WEBVIEW_READY: {
    webview_print_log("WebView ready");
    /* Fires when a document is created, which is about when the browser
       swaps to the composited page surface - the moment the window turns
       white. Put the transparent background back at exactly that point. */
    if (webview_webview2_enabled && (w != NULL) && w->priv.transparent &&
        (w->priv.webview2 != NULL)) {
      WebView2SetTransparent(w->priv.webview2, 1);
    }
    if (webview_webview2_enabled && (w->priv.webview2 != NULL) &&
        (w->priv.webview2->jsToEval != NULL)) {
      char *js = w->priv.webview2->jsToEval;
      w->priv.webview2->jsToEval = NULL;
      webview_eval(w, js);
      free(js);
    }
    return TRUE;
  }
  case WM_WEBVIEW_DISPATCH: {
    webview_dispatch_fn f = (webview_dispatch_fn)wParam;
    void *arg = (void *)lParam;
    (*f)(w, arg);
    return TRUE;
  }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/* ------------------------------------------------------------------------
 * Window icon
 *
 * Three things were wrong before:
 *  - only wc.hIcon (the large icon) was set, so the small icon shown in the
 *    title bar and in the Alt+Tab list stayed the Windows default;
 *  - the icon was looked up relative to the working directory;
 *  - an icon added to the exe afterwards (Resource Hacker) was ignored,
 *    because the code only knew about the compile time id
 *    WEBVIEW_WIN32_ICON_RES.
 * A <link rel="icon"> or <meta> tag in the html never affects the native
 * window icon, it only shows up inside the page.
 * ------------------------------------------------------------------------ */

static BOOL CALLBACK webview_first_icon_proc(HMODULE hModule, LPCSTR lpType,
                                             LPSTR lpName, LONG_PTR lParam) {
  *((LPSTR *)lParam) = lpName;
  return FALSE; /* stop at the first icon group */
}

static HICON webview_load_icon_size(HINSTANCE hInstance, const char *file,
                                    int cx, int cy) {
  HICON icon = NULL;
  LPSTR resName = NULL;
  if ((file != NULL) && (file[0] != '\0')) {
    icon = (HICON)LoadImageA(NULL, file, IMAGE_ICON, cx, cy, LR_LOADFROMFILE);
    if (icon != NULL) {
      return icon;
    }
  }
#if defined(WEBVIEW_WIN32_ICON_RES)
  icon = (HICON)LoadImageA(hInstance, MAKEINTRESOURCEA(WEBVIEW_WIN32_ICON_RES),
                           IMAGE_ICON, cx, cy, 0);
  if (icon != NULL) {
    return icon;
  }
#endif
  /* whatever icon group the exe happens to carry, whichever id it uses */
  EnumResourceNamesA(hInstance, RT_GROUP_ICON,
                     (ENUMRESNAMEPROCA)webview_first_icon_proc,
                     (LONG_PTR)&resName);
  if (resName != NULL) {
    icon = (HICON)LoadImageA(hInstance, resName, IMAGE_ICON, cx, cy, 0);
  }
  return icon;
}

/* Path of an .ico sitting next to the executable (not the working dir). */
static char *webview_icon_beside_exe(char *buffer, size_t size,
                                     const char *name) {
  DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)size);
  char *bs;
  if ((len == 0) || (len >= size)) {
    buffer[0] = '\0';
    return buffer;
  }
  bs = strrchr(buffer, '\\');
  if (bs == NULL) {
    buffer[0] = '\0';
    return buffer;
  }
  bs[1] = '\0';
  if (strlen(buffer) + strlen(name) + 1 > size) {
    buffer[0] = '\0';
    return buffer;
  }
  strcat(buffer, name);
  return buffer;
}

/* The file based route: ask Windows for the icons inside a .exe or .dll.
   Equivalent to the resource lookup above when the file is our own exe, but
   it also works on any other file. Pass NULL for this application's exe. */
static int webview_extract_icons(const char *modulePath, int index, HICON *big,
                                 HICON *small) {
  char self[MAX_PATH];
  UINT n;
  *big = NULL;
  *small = NULL;
  if ((modulePath == NULL) || (modulePath[0] == '\0')) {
    DWORD len = GetModuleFileNameA(NULL, self, MAX_PATH);
    if ((len == 0) || (len >= MAX_PATH)) {
      return -1;
    }
    modulePath = self;
  }
  n = ExtractIconExA(modulePath, index, big, small, 1);
  if ((n == 0) || (n == (UINT)-1)) {
    return -1;
  }
  return ((*big != NULL) || (*small != NULL)) ? 0 : -1;
}

WEBVIEW_API int webview_set_icon_from_module(struct webview *w,
                                             const char *modulePath,
                                             int index) {
  HICON big;
  HICON small;
  if (webview_extract_icons(modulePath, index, &big, &small) != 0) {
    return -1;
  }
  if (big != NULL) {
    SendMessage(w->priv.hwnd, WM_SETICON, ICON_BIG, (LPARAM)big);
  }
  if (small != NULL) {
    SendMessage(w->priv.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small);
  }
  return 0;
}

WEBVIEW_API int webview_set_icon(struct webview *w, const char *icoPath) {
  HICON big = webview_load_icon_size(GetModuleHandle(NULL), icoPath,
                                     GetSystemMetrics(SM_CXICON),
                                     GetSystemMetrics(SM_CYICON));
  HICON small = webview_load_icon_size(GetModuleHandle(NULL), icoPath,
                                       GetSystemMetrics(SM_CXSMICON),
                                       GetSystemMetrics(SM_CYSMICON));
  if ((big == NULL) && (small == NULL)) {
    return -1;
  }
  if (big != NULL) {
    SendMessage(w->priv.hwnd, WM_SETICON, ICON_BIG, (LPARAM)big);
  }
  if (small != NULL) {
    SendMessage(w->priv.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small);
  }
  return 0;
}

static int webview_transparent_at_startup = 0;

WEBVIEW_API void webview_prefer_transparent(int transparent) {
  webview_transparent_at_startup = transparent ? 1 : 0;
}

WEBVIEW_API int webview_init(struct webview *w) {
  WNDCLASSEX wc;
  HINSTANCE hInstance;
  DWORD style;
  RECT clientRect;
  RECT rect;
  char *iconFilename;
  char iconPath[MAX_PATH * 2];
  HICON iconBig;
  HICON iconSmall;
  int policy;

  hInstance = GetModuleHandle(NULL);
  if (hInstance == NULL) {
    return -1;
  }
  /* OLE has to be up before the WebView2 loader is asked for a runtime */
  HRESULT oir = OleInitialize(NULL);
  if (oir != S_OK && oir != S_FALSE) {
    return -1;
  }

  if (webview_transparent_at_startup) {
    /* Has to be in the environment before the WebView2 environment is built,
       which happens in WM_CREATE further down. AARRGGBB, alpha 00. */
    if (getenv("WEBVIEW2_DEFAULT_BACKGROUND_COLOR") == NULL) {
      SetEnvironmentVariableA("WEBVIEW2_DEFAULT_BACKGROUND_COLOR", "00FFFFFF");
      webview_print_log("WEBVIEW2_DEFAULT_BACKGROUND_COLOR=00FFFFFF "
                        "(transparent first frame)");
    }
    w->priv.transparent = 1;
  }

  policy = webview_resolve_policy();

  if (policy == WEBVIEW_BACKEND_IE) {
    webview_webview2_enabled = 0;
    webview_print_log("backend policy: MSHTML/IE requested");
  } else {
    webview_webview2_enabled = WebView2Enable();
  }

  if (!webview_webview2_enabled) {
    if (policy == WEBVIEW_BACKEND_WEBVIEW2) {
      /* Do not open a silently broken IE window: say what is missing. */
      webview_report_no_webview2();
      OleUninitialize();
      return -2;
    }
    /* Falling back to MSHTML/IE. This is a big deal - no transparency, no
       modern CSS or JS, none of w.browser applies - so say so plainly
       rather than opening a window that quietly behaves nothing like the
       one the developer tested. Set the backend policy to WEBVIEW2 to make
       this a hard failure instead. */
    webview_print_log("*** WebView2 unavailable - falling back to MSHTML/IE.");
    webview_print_log("*** Transparency and modern CSS will NOT work "
                      "(JavaScript is translated).");
    webview_print_log(WebView2Diagnostic());
    /* The old MSHTML control needs this registry tweak, but a failure here
       (locked down machine, roaming profile) must not kill the app. */
    if (webview_fix_ie_compat_mode() < 0) {
      webview_print_log("could not set the IE feature control registry key");
    }
    /* scripts the control loads go through the ES3 translator */
    webview_legacy_register();
  }

  iconFilename = getenv("WEBVIEW_WIN32_ICON");
  if (iconFilename == NULL) {
    webview_icon_beside_exe(iconPath, sizeof(iconPath), "webview.ico");
    iconFilename = iconPath;
  }
  iconBig = webview_load_icon_size(hInstance, iconFilename,
                                   GetSystemMetrics(SM_CXICON),
                                   GetSystemMetrics(SM_CYICON));
  iconSmall = webview_load_icon_size(hInstance, iconFilename,
                                     GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON));
  if ((iconBig == NULL) && (iconSmall == NULL)) {
    /* last resort: whatever icon the exe file shows in Explorer */
    webview_extract_icons(NULL, 0, &iconBig, &iconSmall);
  }

  ZeroMemory(&wc, sizeof(WNDCLASSEX));
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.hInstance = hInstance;
  wc.lpfnWndProc = wndproc;
  wc.lpszClassName = classname;
  wc.hIcon = iconBig;
  wc.hIconSm = iconSmall; /* title bar / Alt+Tab, was never set before */
  RegisterClassEx(&wc);

  style = WS_OVERLAPPEDWINDOW;
  if (!w->resizable) {
    style = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
  }

  rect.left = 0;
  rect.top = 0;
  rect.right = w->width;
  rect.bottom = w->height;
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

  GetClientRect(GetDesktopWindow(), &clientRect);
  int left = (clientRect.right / 2) - ((rect.right - rect.left) / 2);
  int top = (clientRect.bottom / 2) - ((rect.bottom - rect.top) / 2);
  rect.right = rect.right - rect.left + left;
  rect.left = left;
  rect.bottom = rect.bottom - rect.top + top;
  rect.top = top;

  w->priv.hwnd =
      CreateWindowEx(0, classname, w->title, style | WS_CLIPCHILDREN,
                     rect.left, rect.top,
                     rect.right - rect.left, rect.bottom - rect.top,
                     HWND_DESKTOP, NULL, hInstance, (void *)w);
  if (w->priv.hwnd == 0) {
    OleUninitialize();
    return -1;
  }

  SetWindowLongPtr(w->priv.hwnd, GWLP_USERDATA, (LONG_PTR)w);

  /* The class icons cover most cases, but an explicit WM_SETICON also fixes
     windows created before the class was registered and refreshes the
     taskbar button. */
  if (iconBig != NULL) {
    SendMessage(w->priv.hwnd, WM_SETICON, ICON_BIG, (LPARAM)iconBig);
  }
  if (iconSmall != NULL) {
    SendMessage(w->priv.hwnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);
  }

  if (!webview_webview2_enabled) {
    DisplayHTMLPage(w);
  }

  SetWindowText(w->priv.hwnd, w->title);

  /* w.window.* - the ones that do not need the browser. Done before the
     window is shown so nothing flickers. Transparency waits for the page. */
  webview_apply_window_options(w);

  ShowWindow(w->priv.hwnd, SW_SHOWDEFAULT);
  UpdateWindow(w->priv.hwnd);
  SetFocus(w->priv.hwnd);

  return 0;
}

WEBVIEW_API int webview_loop(struct webview *w, int blocking) {
  MSG msg;
  if (blocking) {
    if (GetMessage(&msg, 0, 0, 0)<0) return 0;
  } else {
    if (!PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) return 0;
  }
  /* F12 on the IE backend: the devtools drawn into the page */
  if (!webview_webview2_enabled && (msg.message == WM_KEYDOWN) &&
      (msg.wParam == VK_F12) && webview_ie_devtools_allowed(w) &&
      ((msg.hwnd == w->priv.hwnd) || IsChild(w->priv.hwnd, msg.hwnd))) {
    webview_ie_devtools(w, 1);
    return 0;
  }
  switch (msg.message) {
  case WM_QUIT:
    return -1;
  case WM_COMMAND:
  case WM_KEYDOWN:
  case WM_KEYUP: 
    if (!webview_webview2_enabled) {
      HRESULT r = S_OK;
      IWebBrowser2 *webBrowser2;
      IOleObject *browser = *w->priv.browser;
      if (browser->lpVtbl->QueryInterface(browser, iid_unref(&IID_IWebBrowser2),
                                          (void **)&webBrowser2) == S_OK) {
        IOleInPlaceActiveObject *pIOIPAO;
        if (browser->lpVtbl->QueryInterface(
                browser, iid_unref(&IID_IOleInPlaceActiveObject),
                (void **)&pIOIPAO) == S_OK) {
          r = pIOIPAO->lpVtbl->TranslateAccelerator(pIOIPAO, &msg);
          pIOIPAO->lpVtbl->Release(pIOIPAO);
        }
        webBrowser2->lpVtbl->Release(webBrowser2);
      }
      if (r != S_FALSE) {
        return 0;
      }
    }
    break;
  }
  TranslateMessage(&msg);
  DispatchMessage(&msg);
  return 0;
}

/* --------------------------------------------------------------------------
 * Frameless and transparent windows
 *
 * Two independent switches, both safe to call straight after webview_init():
 *
 *   webview_set_frameless(&w, 1);     no title bar, no border
 *   webview_set_transparent(&w, 1);   the page composites onto the desktop
 *
 * For transparency you need BOTH halves. This turns on the window's alpha
 * channel and tells WebView2 to paint with a transparent background; your
 * page still has to actually be transparent:
 *
 *   html, body { background: transparent; margin: 0; }
 *
 * Anything the page paints is opaque as usual, so a rounded, translucent
 * "window" is just a div with a background and a border-radius.
 *
 * Losing the frame means losing the things the frame did for you: dragging,
 * resizing, snap, the system menu. You said you would handle those - the
 * usual route is WM_NCHITTEST returning HTCAPTION / HTBOTTOMRIGHT, or
 * -webkit-app-region: drag in the page plus a WM_NCLBUTTONDOWN forward.
 * ------------------------------------------------------------------------*/

WEBVIEW_API void webview_set_frameless(struct webview *w, int frameless) {
  LONG_PTR style = GetWindowLongPtr(w->priv.hwnd, GWL_STYLE);
  /* Set before the frame change below, which is what sends WM_NCCALCSIZE. */
  w->priv.hidden_frame = 0;
  if (frameless && webview_mode_is_glass(w->window.transparent)) {
    /* Glass needs a frame DWM can see: it puts no material behind a plain
       WS_POPUP window. Keep the caption and sizing styles and hide them in
       WM_NCCALCSIZE instead - see wv_glass.inl. */
    style &= ~WS_POPUP;
    style |= WS_OVERLAPPEDWINDOW;
    if (!w->resizable) {
      style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    w->priv.hidden_frame = 1;
  } else if (frameless) {
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
               WS_SYSMENU);
    style |= WS_POPUP;
  } else {
    style &= ~WS_POPUP;
    style |= WS_OVERLAPPEDWINDOW;
  }
  SetWindowLongPtr(w->priv.hwnd, GWL_STYLE, style);
  /* SWP_FRAMECHANGED is what makes Windows recalculate the non client area */
  SetWindowPos(w->priv.hwnd, NULL, 0, 0, 0, 0,
               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE);
}

/* Paint the empty area of the page in a colour of your choice. Mostly useful
   for finding out whether the WebView2 half of transparency is working:

       webview_set_background(&w, 0, 0, 255, 255);   -> a blue window
                                                        means the call works,
                                                        so any remaining white
                                                        is coming from the page
       webview_set_background(&w, 0, 0, 0, 0);       -> transparent

   Alpha only does something once the window itself has an alpha channel, see
   webview_set_transparent(). */
WEBVIEW_API int webview_set_background(struct webview *w, uint8_t r, uint8_t g,
                                       uint8_t b, uint8_t a) {
  if (!webview_webview2_enabled) {
    return -1;
  }
  if (w->priv.webview2 != NULL) {
    w->priv.webview2->transparent = (a == 0);
  }
  return WebView2SetBackground(w->priv.webview2, (BYTE)r, (BYTE)g, (BYTE)b,
                               (BYTE)a);
}

/* Per pixel alpha, the combination that is actually known to work with a
   windowed WebView2:

     1. DwmEnableBlurBehindWindow with an EMPTY blur region - "give this
        window an alpha channel, do not blur anything";
     2. WM_ERASEBKGND returning 1 - nothing may paint a solid background
        behind the browser (handled in wndproc above);
     3. ICoreWebView2Controller2::put_DefaultBackgroundColor with alpha 0 -
        the browser itself paints nothing where the page is transparent;
     4. html, body { background: transparent; } in your CSS.

   All four are required and any one of them missing gives you a plain white
   window with no error anywhere. (2) is the one everybody misses.

   Deliberately NOT done here: SetWindowCompositionAttribute accent policies
   and DwmExtendFrameIntoClientArea. They look like they belong, but stacking
   them on top of the blur-behind call changes what DWM composites and can
   undo the transparency. Acrylic is a separate call below. */
WEBVIEW_API int webview_set_transparent(struct webview *w, int transparent) {
  typedef HRESULT(WINAPI * DwmEnableBlurBehindWindowFn)(HWND,
                                                        const DWM_BLURBEHIND *);
  HMODULE dwm;
  DwmEnableBlurBehindWindowFn enableBlur;
  DWM_BLURBEHIND bb;
  HRGN region;
  char buffer[128];
  int r = 0;

  w->priv.transparent = transparent ? 1 : 0;

  dwm = LoadLibraryA("dwmapi.dll");
  if (dwm == NULL) {
    webview_print_log("transparency: dwmapi.dll is unavailable");
    return -1;
  }
  enableBlur = (DwmEnableBlurBehindWindowFn)GetProcAddress(
      dwm, "DwmEnableBlurBehindWindow");
  if (enableBlur == NULL) {
    webview_print_log("transparency: DwmEnableBlurBehindWindow is missing");
    return -1;
  }
  region = CreateRectRgn(0, 0, -1, -1); /* empty region: alpha, no blur */
  ZeroMemory(&bb, sizeof(bb));
  bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
  bb.fEnable = transparent ? TRUE : FALSE;
  bb.hRgnBlur = region;
  sprintf(buffer, "transparency: DwmEnableBlurBehindWindow -> 0x%08lX",
          (unsigned long)enableBlur(w->priv.hwnd, &bb));
  webview_print_log(buffer);
  if (region != NULL) {
    DeleteObject(region);
  }
  /* deliberately not FreeLibrary: the window goes on using it */

  /* nothing may paint a background either */
  SetClassLongPtr(w->priv.hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)NULL);

  if (!webview_webview2_enabled) {
    webview_print_log("transparency is not supported on the MSHTML fallback");
    return -1;
  }
  if (WebView2SetTransparent(w->priv.webview2, transparent) != 0) {
    r = -1;
  }
  InvalidateRect(w->priv.hwnd, NULL, TRUE);
  return r;
}

/* Windows 10/11 frosted glass behind the page instead of the bare desktop.
   This one does use the undocumented accent policy, because that is the only
   way to ask for acrylic. */
WEBVIEW_API int webview_set_acrylic(struct webview *w, int enabled) {
  typedef enum {
    WV_ACCENT_DISABLED = 0,
    WV_ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
  } WV_ACCENT_STATE;
  typedef struct {
    WV_ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor; /* AABBGGRR */
    DWORD AnimationId;
  } WV_ACCENT_POLICY;
  typedef struct {
    DWORD Attrib; /* 19 = WCA_ACCENT_POLICY */
    PVOID pvData;
    SIZE_T cbData;
  } WV_WINCOMPATTRDATA;
  typedef BOOL(WINAPI * SetWindowCompositionAttributeFn)(HWND,
                                                         WV_WINCOMPATTRDATA *);
  HMODULE user32 = GetModuleHandleA("user32.dll");
  SetWindowCompositionAttributeFn setAttr;
  WV_ACCENT_POLICY accent;
  WV_WINCOMPATTRDATA data;

  w->priv.transparent = enabled ? 1 : 0;
  setAttr = (user32 != NULL)
                ? (SetWindowCompositionAttributeFn)GetProcAddress(
                      user32, "SetWindowCompositionAttribute")
                : NULL;
  if (setAttr == NULL) {
    webview_print_log("acrylic: SetWindowCompositionAttribute is missing");
    return -1;
  }
  ZeroMemory(&accent, sizeof(accent));
  accent.AccentState =
      enabled ? WV_ACCENT_ENABLE_ACRYLICBLURBEHIND : WV_ACCENT_DISABLED;
  accent.AccentFlags = 2;
  accent.GradientColor = 0x01000000; /* mostly transparent tint */
  data.Attrib = 19;
  data.pvData = &accent;
  data.cbData = sizeof(accent);
  if (!setAttr(w->priv.hwnd, &data)) {
    webview_print_log("acrylic: SetWindowCompositionAttribute failed");
    return -1;
  }
  SetClassLongPtr(w->priv.hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)NULL);
  if (webview_webview2_enabled) {
    return WebView2SetTransparent(w->priv.webview2, enabled);
  }
  return -1;
}

/* --- the route that does not need an alpha channel at all ----------------
 *
 * Why this exists: a WINDOWED WebView2 (the kind this library creates, a
 * child HWND) draws into its own opaque surface. Giving the top level window
 * an alpha channel does not make that child surface translucent, which is why
 * webview_set_transparent() can do everything right and still leave you
 * looking at white. Per pixel alpha with WebView2 really needs visual
 * hosting - CreateCoreWebView2CompositionController plus DirectComposition -
 * which is a different way of embedding the browser entirely.
 *
 * A layered window sidesteps the whole question: every pixel of exactly the
 * key colour is punched out of the finished window, child windows included.
 * So we do not need any alpha anywhere - we just need the page to paint the
 * key colour.
 *
 *     webview_set_transparent_colorkey(&w, 255, 0, 255);
 *
 *     html, body { background: #ff00ff; }     <- in your CSS
 *
 * We also set the WebView2 default background to the same colour, so it
 * still works if the page leaves it transparent - but the CSS line is the
 * part that does not depend on anything.
 *
 * The trade off is 1 bit transparency: a pixel is in or out, nothing in
 * between. No soft shadows, and antialiased edges (rounded corners, text over
 * the key colour) fringe towards magenta. Pick a key you never draw.
 */
WEBVIEW_API int webview_set_transparent_colorkey(struct webview *w, uint8_t r,
                                                 uint8_t g, uint8_t b) {
  LONG_PTR ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  char buffer[128];
  w->priv.transparent = 1;
  SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
  if (!SetLayeredWindowAttributes(w->priv.hwnd, RGB(r, g, b), 255,
                                  LWA_COLORKEY)) {
    sprintf(buffer, "colorkey: SetLayeredWindowAttributes failed (%lu)",
            (unsigned long)GetLastError());
    webview_print_log(buffer);
    return -1;
  }
  sprintf(buffer, "colorkey: keying out RGB(%u,%u,%u) - your CSS should paint "
                  "that colour on html and body",
          (unsigned)r, (unsigned)g, (unsigned)b);
  webview_print_log(buffer);
  SetClassLongPtr(w->priv.hwnd, GCLP_HBRBACKGROUND,
                  (LONG_PTR)GetStockObject(NULL_BRUSH));
  if (!webview_webview2_enabled) {
    return -1;
  }
  /* belt and braces: also ask WebView2 to paint the key colour where the
     page paints nothing */
  return WebView2SetBackground(w->priv.webview2, (BYTE)r, (BYTE)g, (BYTE)b,
                               255);
}

WEBVIEW_API void webview_set_title(struct webview *w, const char *title) {
  SetWindowText(w->priv.hwnd, title);
}

WEBVIEW_API void webview_set_fullscreen(struct webview *w, int fullscreen) {
  if (w->priv.is_fullscreen == !!fullscreen) {
    return;
  }
  if (w->priv.is_fullscreen == 0) {
    /* A maximized window has to be restored first: otherwise the size saved
       below is the maximized one, and coming back out of fullscreen leaves
       a window that looks maximized but is not, with the wrong restore
       button. Maximized again on the way out. Same as Chromium does. */
    w->priv.saved_maximized = IsZoomed(w->priv.hwnd) ? 1 : 0;
    if (w->priv.saved_maximized) {
      SendMessage(w->priv.hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    }
    w->priv.saved_style = GetWindowLong(w->priv.hwnd, GWL_STYLE);
    w->priv.saved_ex_style = GetWindowLong(w->priv.hwnd, GWL_EXSTYLE);
    GetWindowRect(w->priv.hwnd, &w->priv.saved_rect);
  }
  w->priv.is_fullscreen = !!fullscreen;
  if (fullscreen) {
    MONITORINFO monitor_info;
    SetWindowLong(w->priv.hwnd, GWL_STYLE,
                  w->priv.saved_style & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowLong(w->priv.hwnd, GWL_EXSTYLE,
                  w->priv.saved_ex_style &
                      ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
                        WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfo(MonitorFromWindow(w->priv.hwnd, MONITOR_DEFAULTTONEAREST),
                   &monitor_info);
    RECT r;
    r.left = monitor_info.rcMonitor.left;
    r.top = monitor_info.rcMonitor.top;
    r.right = monitor_info.rcMonitor.right;
    r.bottom = monitor_info.rcMonitor.bottom;
    SetWindowPos(w->priv.hwnd, NULL, r.left, r.top, r.right - r.left,
                 r.bottom - r.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  } else {
    SetWindowLong(w->priv.hwnd, GWL_STYLE, w->priv.saved_style);
    SetWindowLong(w->priv.hwnd, GWL_EXSTYLE, w->priv.saved_ex_style);
    SetWindowPos(w->priv.hwnd, NULL, w->priv.saved_rect.left,
                 w->priv.saved_rect.top,
                 w->priv.saved_rect.right - w->priv.saved_rect.left,
                 w->priv.saved_rect.bottom - w->priv.saved_rect.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (w->priv.saved_maximized) {
      SendMessage(w->priv.hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }
  }
}

WEBVIEW_API void webview_set_color(struct webview *w, uint8_t r, uint8_t g,
                                   uint8_t b, uint8_t a) {
  HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
  SetClassLongPtr(w->priv.hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
}

WEBVIEW_API void webview_terminate(struct webview *w) { PostQuitMessage(0); }

WEBVIEW_API void webview_exit(struct webview *w) {
  int i;
  for (i = 0; i < w->priv.binding_count; i++) {
    free(w->priv.bindings[i].name);
  }
  free(w->priv.bindings);
  w->priv.bindings = NULL;
  w->priv.binding_count = 0;
  w->priv.binding_capacity = 0;
  DestroyWindow(w->priv.hwnd);
  OleUninitialize();
}
