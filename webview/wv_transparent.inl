/* ---------------------------------------------------------------------------
 * wv_transparent.inl - transparent windows, in one call
 *
 * The Neutralino equivalent of `modes.window.transparent: true` is:
 *
 *     webview_prefer_transparent(1);                    // before init
 *     webview_init(&w);
 *     webview_window_transparency(&w, WEBVIEW_WINDOW_ALPHA);
 *
 * plus, in your CSS:   html, body { background: transparent; }
 *
 * Three routes are available, because no single one works everywhere:
 *
 *   WEBVIEW_WINDOW_ALPHA     real per-pixel alpha. Soft shadows, antialiased
 *                            rounded corners, rgba() partial transparency.
 *                            This is the one that matches Neutralino.
 *   WEBVIEW_WINDOW_COLORKEY  a layered window punches out one exact colour.
 *                            1-bit: a pixel is in or out. No soft edges, but
 *                            it does not depend on the browser painting
 *                            transparent pixels, so it survives runtimes
 *                            where alpha is broken.
 *   WEBVIEW_WINDOW_ACRYLIC   Windows 10/11 frosted glass behind the page,
 *                            through the old accent policy.
 *
 * And a family of blurred ones - the desktop behind the window, frosted -
 * built on the alpha route. They live in wv_glass.inl:
 *
 *   WEBVIEW_WINDOW_GLASS     see-through blur (Desktop Acrylic on 11 22H2+)
 *   WEBVIEW_WINDOW_MICA      Windows 11 Mica, tinted by the wallpaper
 *   WEBVIEW_WINDOW_MICA_ALT  Windows 11 Mica Alt
 *   WEBVIEW_WINDOW_BLUR      accent-policy blur, any Windows 10/11
 *
 * When none of them work, webview_transparency_report() says which layer
 * failed instead of leaving you guessing. It reads the background colour BACK
 * out of WebView2, so it can tell "the runtime refused our value" apart from
 * "the runtime took it and painted white anyway".
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

enum webview_window_transparency {
  WEBVIEW_WINDOW_OPAQUE = 0,
  WEBVIEW_WINDOW_ALPHA = 1,
  WEBVIEW_WINDOW_COLORKEY = 2,
  WEBVIEW_WINDOW_ACRYLIC = 3,
  WEBVIEW_WINDOW_ALPHA_PLAIN = 4,
  /* blurred backdrops, see wv_glass.inl */
  WEBVIEW_WINDOW_GLASS = 5,
  WEBVIEW_WINDOW_MICA = 6,
  WEBVIEW_WINDOW_MICA_ALT = 7,
  WEBVIEW_WINDOW_BLUR = 8
};

static int webview_transparency_mode = WEBVIEW_WINDOW_OPAQUE;
static BYTE webview_key_r = 255;
static BYTE webview_key_g = 0;
static BYTE webview_key_b = 255;

/* Which colour WEBVIEW_WINDOW_COLORKEY punches out. Magenta by default,
   because almost nothing draws it by accident. Call before
   webview_window_transparency(). Your CSS must paint the same colour. */
WEBVIEW_API void webview_set_colorkey(uint8_t r, uint8_t g, uint8_t b) {
  webview_key_r = (BYTE)r;
  webview_key_g = (BYTE)g;
  webview_key_b = (BYTE)b;
}

/* Read the background colour back out of WebView2. This is the difference
   between "our call was rejected" and "our call was accepted and ignored".
   Returns 0 on success. */
WEBVIEW_API int webview_get_background(struct webview *w, uint8_t *r,
                                       uint8_t *g, uint8_t *b, uint8_t *a) {
  ICoreWebView2Controller2 *controller2 = NULL;
  COREWEBVIEW2_COLOR color;
  HRESULT hr;

  if ((w == NULL) || !webview_webview2_enabled ||
      (w->priv.webview2 == NULL) || (w->priv.webview2->controller == NULL)) {
    return -1;
  }
  hr = w->priv.webview2->controller->lpVtbl->QueryInterface(
      w->priv.webview2->controller, iid_unref(&IID_ICoreWebView2Controller2),
      (void **)&controller2);
  if (FAILED(hr) || (controller2 == NULL)) {
    return -1;
  }
  ZeroMemory(&color, sizeof(color));
  hr = controller2->lpVtbl->get_DefaultBackgroundColor(controller2, &color);
  controller2->lpVtbl->Release(controller2);
  if (FAILED(hr)) {
    return -1;
  }
  if (r != NULL) { *r = color.R; }
  if (g != NULL) { *g = color.G; }
  if (b != NULL) { *b = color.B; }
  if (a != NULL) { *a = color.A; }
  return 0;
}

/* Major version of the installed WebView2 runtime, or 0 if unknown. Parsed
   out of the loader's diagnostic line ("WebView2 runtime 145.0.3770.0"). */
WEBVIEW_API int webview_runtime_major(void) {
  const char *diag = WebView2Diagnostic();
  const char *p;
  if (diag == NULL) {
    return 0;
  }
  p = strstr(diag, "runtime ");
  if (p == NULL) {
    return 0;
  }
  p += 8;
  if ((*p < '0') || (*p > '9')) {
    return 0;
  }
  return atoi(p);
}

/* 1 when the installed runtime is in the range that lost transparency.
   144.x and 145.x render an opaque background whatever alpha you ask for -
   WebView2Feedback #5481 and #5492, both filed in January 2026. Nothing you
   write in C can work around that; pin an older runtime or use the colour
   key route. */
WEBVIEW_API int webview_runtime_transparency_broken(void) {
  int major = webview_runtime_major();
  return ((major == 144) || (major == 145)) ? 1 : 0;
}

/* Put the window in layered mode before asking DWM for alpha.
   On some Windows 11 builds the blur-behind trick alone does not grant a
   usable alpha channel, and the window stays solid however correct everything
   else is. Adding WS_EX_LAYERED first fixes it. The colour key we install is
   deliberately one the page never paints, so nothing is punched out by it -
   we only want the layered composition path, not the keying. */
static void webview_prime_layered(struct webview *w) {
  LONG_PTR ex;
  if (w == NULL) {
    return;
  }
  ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
  SetLayeredWindowAttributes(w->priv.hwnd,
                             RGB(webview_key_r, webview_key_g, webview_key_b),
                             255, LWA_COLORKEY);
}

/* Windows 11 draws a "system backdrop" behind the client area of any window
   that still has a caption. It sits between your alpha pixels and the desktop,
   so a framed window can be perfectly transparent and still look solid. Ask
   DWM for DWMSBT_NONE so there is nothing behind us but the desktop.
   Harmless on Windows 10 and earlier: the attribute is simply rejected. */
static void webview_disable_system_backdrop(struct webview *w) {
  typedef HRESULT(WINAPI * DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID,
                                                    DWORD);
  HMODULE dwm;
  DwmSetWindowAttribute_t set_attribute;
  int backdrop = 1;  /* DWMSBT_NONE */
  int dark = 1;      /* DWMWA_USE_IMMERSIVE_DARK_MODE, stops a white caption */

  if (w == NULL) {
    return;
  }
  dwm = LoadLibraryA("dwmapi.dll");
  if (dwm == NULL) {
    return;
  }
  set_attribute =
      (DwmSetWindowAttribute_t)(void *)GetProcAddress(dwm,
                                                      "DwmSetWindowAttribute");
  if (set_attribute != NULL) {
    /* 38 = DWMWA_SYSTEMBACKDROP_TYPE, 20 = DWMWA_USE_IMMERSIVE_DARK_MODE */
    set_attribute(w->priv.hwnd, 38, &backdrop, sizeof(backdrop));
    set_attribute(w->priv.hwnd, 20, &dark, sizeof(dark));
  }
  FreeLibrary(dwm);
}

/* Poke the controller so the browser recomposites. Some runtimes only pick
   up a new background colour on the next layout pass, which is why a window
   that is stubbornly white sometimes fixes itself the moment you drag its
   edge. This does the drag for you. */
static void webview_recomposite(struct webview *w) {
  RECT rect;
  RECT nudged;
  if ((w == NULL) || !webview_webview2_enabled ||
      (w->priv.webview2 == NULL) || (w->priv.webview2->controller == NULL)) {
    return;
  }
  GetClientRect(w->priv.hwnd, &rect);
  nudged = rect;
  if (nudged.right > nudged.left) {
    nudged.right -= 1;
  }
  WebView2SetBounds(w->priv.webview2, nudged);
  WebView2SetBounds(w->priv.webview2, rect);
  InvalidateRect(w->priv.hwnd, NULL, TRUE);
}

/* Erase the client area to something that disappears.
 *
 * Returning 1 from WM_ERASEBKGND without painting is what stops a solid
 * background appearing behind the browser - but it also means nothing ever
 * clears the window's surface. Resize a transparent window and the pixels
 * that were there before simply stay: old borders and caption fragments
 * smeared across the desktop, which is what "resize trails" are.
 *
 * So paint, but paint something that vanishes: black. A GDI fill does not
 * touch the alpha byte, so it stays at zero, and DWM reads zero alpha as
 * fully transparent. The fill is invisible and it wipes what was underneath.
 *
 * NOT the colour key, however tempting. Filling the client area with the key
 * colour makes the whole window colour-keyed, and on a layered window keyed
 * pixels do not receive mouse input - the entire window turns click-through
 * and the app becomes impossible to interact with. Black is transparent to
 * DWM but is not the key, so hit testing still works normally. */
static int webview_erase_armed = 0;

static void webview_erase_transparent(struct webview *w, HDC dc) {
  RECT rect;

  if ((w == NULL) || (dc == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  /* Two conditions, both necessary.
     Not before the page is up: until then transparency has not landed, so
     the fill is not invisible, it is a black box on screen.
     And only while it is actually needed - during a drag or a frame rebuild.
     Trails only appear when the window changes size or frame; the rest of
     the time paint nothing, exactly as before. */
  if (!w->window.webview_loaded || !webview_erase_armed) {
    return;
  }
  GetClientRect(w->priv.hwnd, &rect);
  FillRect(dc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

/* THE call. One mode in, 0 or -1 out. */
WEBVIEW_API int webview_window_transparency(struct webview *w, int mode) {
  int result;
  int previous;

  if (w == NULL) {
    return -1;
  }
  previous = webview_transparency_mode;
  webview_transparency_mode = mode;

  /* Leaving a mode that left something behind in DWM. Both are no-ops when
     nothing was set, so the retries re-applying the same mode cost nothing. */
  if ((previous == WEBVIEW_WINDOW_ACRYLIC) && (mode != WEBVIEW_WINDOW_ACRYLIC)) {
    webview_set_acrylic(w, 0);
  }
  if (!webview_mode_is_glass(mode)) {
    webview_glass_clear(w, mode);
  }

  switch (mode) {
  case WEBVIEW_WINDOW_ALPHA:
    webview_disable_system_backdrop(w);
    webview_prime_layered(w);
    result = webview_set_transparent(w, 1);
    break;
  case WEBVIEW_WINDOW_ALPHA_PLAIN:
    /* Same thing without the layered step, so you can find out whether your
       machine needs it. */
    webview_disable_system_backdrop(w);
    result = webview_set_transparent(w, 1);
    break;
  case WEBVIEW_WINDOW_COLORKEY:
    result = webview_set_transparent_colorkey(w, webview_key_r, webview_key_g,
                                              webview_key_b);
    break;
  case WEBVIEW_WINDOW_ACRYLIC:
    result = webview_set_acrylic(w, 1);
    break;
  case WEBVIEW_WINDOW_GLASS:
  case WEBVIEW_WINDOW_MICA:
  case WEBVIEW_WINDOW_MICA_ALT:
  case WEBVIEW_WINDOW_BLUR:
    result = webview_glass_apply(w, mode);
    break;
  default: {
    LONG_PTR ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
    SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
    result = webview_set_transparent(w, 0);
    break;
  }
  }
  webview_recomposite(w);
  return result;
}

/* Fill `buffer` with a human readable account of every layer transparency
   depends on, and what to try next. Print it, put it in a message box, or
   watch it in Sysinternals DebugView. Returns buffer. */
WEBVIEW_API const char *webview_transparency_report(struct webview *w,
                                                    char *buffer,
                                                    size_t size) {
  const char *mode_name = "opaque";
  const char *verdict;
  uint8_t r = 0, g = 0, b = 0, a = 255;
  int have_bg;
  int have_controller;
  int broken = webview_runtime_transparency_broken();
  LONG_PTR ex;
  HBRUSH brush;

  if ((buffer == NULL) || (size == 0)) {
    return buffer;
  }
  if (w == NULL) {
    snprintf(buffer, size, "no window");
    return buffer;
  }

  switch (webview_transparency_mode) {
  case WEBVIEW_WINDOW_ALPHA: mode_name = "alpha (layered + DWM)"; break;
  case WEBVIEW_WINDOW_ALPHA_PLAIN: mode_name = "alpha (DWM only)"; break;
  case WEBVIEW_WINDOW_COLORKEY: mode_name = "colour key (layered)"; break;
  case WEBVIEW_WINDOW_ACRYLIC: mode_name = "acrylic"; break;
  case WEBVIEW_WINDOW_GLASS: mode_name = "glass (blurred desktop)"; break;
  case WEBVIEW_WINDOW_MICA: mode_name = "glass: Mica"; break;
  case WEBVIEW_WINDOW_MICA_ALT: mode_name = "glass: Mica Alt"; break;
  case WEBVIEW_WINDOW_BLUR: mode_name = "glass: accent blur"; break;
  default: break;
  }

  have_controller = (webview_webview2_enabled &&
                     (w->priv.webview2 != NULL) &&
                     (w->priv.webview2->controller != NULL));
  have_bg = (webview_get_background(w, &r, &g, &b, &a) == 0);
  ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  brush = (HBRUSH)GetClassLongPtr(w->priv.hwnd, GCLP_HBRBACKGROUND);

  /* Work out the single most useful next step. */
  if (!webview_webview2_enabled) {
    verdict = "MSHTML/IE fallback is in use - it cannot do transparency at\n"
              "  all. Install the WebView2 runtime.";
  } else if (!have_controller) {
    verdict = "the browser does not exist yet. WebView2 is created\n"
              "  asynchronously, so a report taken straight after\n"
              "  webview_init() tells you nothing - the message loop has not\n"
              "  run once. Ask for it after the page has loaded instead.";
  } else if (!have_bg) {
    verdict = "the browser exists but would not tell us its background\n"
              "  colour, so the runtime is older than 1.0.774.44 (mid 2020),\n"
              "  or QueryInterface for ICoreWebView2Controller2 failed.";
  } else if (broken) {
    verdict = "the runtime is 144.x or 145.x, which lost transparency\n"
              "  (WebView2Feedback #5481, #5492). No C code can fix this.\n"
              "  Either update past it, pin an older runtime with\n"
              "  WEBVIEW2_BROWSER_EXECUTABLE_FOLDER, or switch to\n"
              "  WEBVIEW_WINDOW_COLORKEY, which does not need the browser to\n"
              "  paint transparent pixels.";
  } else if (a != 0 && ((webview_transparency_mode == WEBVIEW_WINDOW_ALPHA) ||
                       (webview_transparency_mode ==
                        WEBVIEW_WINDOW_ALPHA_PLAIN) ||
                       webview_mode_is_glass(webview_transparency_mode))) {
    verdict = "we asked for alpha 0 and the runtime kept an opaque colour.\n"
              "  That is the runtime overriding us - try the colour key.";
  } else if (webview_transparency_mode == WEBVIEW_WINDOW_OPAQUE) {
    verdict = "transparency was never switched on. Call\n"
              "  webview_window_transparency(&w, WEBVIEW_WINDOW_ALPHA).";
  } else {
    verdict = "every C-side layer reports success. If the window is still\n"
              "  solid, the colour is coming from your CSS - check that\n"
              "  html AND body are both transparent, and that no element\n"
              "  covers the page with an opaque background.";
  }

  snprintf(buffer, size,
           "transparency report\n"
           "-------------------\n"
           "  backend .............. %s\n"
           "  runtime .............. %s%s\n"
           "  requested mode ....... %s\n"
           "  browser created ...... %s\n"
           "  WebView2 background .. %s\n"
           "  WM_ERASEBKGND ........ %s\n"
           "  WS_EX_LAYERED ........ %s\n"
           "  class brush .......... %s\n"
           "\n"
           "  next step: %s\n",
           webview_webview2_enabled ? "WebView2" : "MSHTML/IE",
           WebView2Diagnostic(),
           broken ? "   << known bad for transparency" : "",
           mode_name,
           have_controller ? "yes" : "NOT YET - report is premature",
           have_bg ? "readable" : "unavailable",
           w->priv.transparent ? "swallowed (good)" : "passed to DefWindowProc",
           (ex & WS_EX_LAYERED) ? "on" : "off",
           (brush == NULL) ? "none (good)" : "set - it will paint over you",
           verdict);

  if (webview_mode_is_glass(webview_transparency_mode)) {
    size_t used = strlen(buffer);
    if (used + 1 < size) {
      snprintf(buffer + used, size - used,
               "\n  windows .............. %s\n"
               "  glass ................ %s\n"
               "  hidden frame ......... %s\n"
               "  if it is clear but not blurred, try webview_glass_tuning()\n"
               "  - layered off, or extend on - and apply the mode again.\n",
               (webview_glass_blocked() != NULL)
                   ? webview_glass_blocked()
                   : "will draw materials",
               webview_glass_info(),
               w->priv.hidden_frame ? "yes (WS_CAPTION kept, frame hidden)"
                                    : "no");
    }
  }

  if (have_bg) {
    char line[96];
    snprintf(line, sizeof(line),
             "  WebView2 reports rgba(%u,%u,%u,%u)%s\n",
             (unsigned)r, (unsigned)g, (unsigned)b, (unsigned)a,
             (a == 0) ? " - transparent, as asked" : " - OPAQUE");
    webview_print_log(line);
  }
  webview_print_log(buffer);
  return buffer;
}
