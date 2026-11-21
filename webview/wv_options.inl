/* ---------------------------------------------------------------------------
 * wv_options.inl - window settings as separate toggles
 *
 * Fill in w.window before webview_init() and forget about it:
 *
 *     struct webview w;
 *     memset(&w, 0, sizeof(w));
 *     w.title  = "my app";
 *     w.url    = "...";
 *     w.window.transparent   = WEBVIEW_WINDOW_ALPHA;
 *     w.window.frameless     = 1;
 *     w.window.always_on_top = 1;
 *     w.window.center        = 1;
 *     webview_init(&w);
 *
 * Every field is an independent switch and every one defaults to off, so the
 * memset that every example already does is enough to opt out of all of them.
 *
 * webview_init() applies the window-side options immediately and the
 * transparency once the browser is actually up - which is the timing that
 * makes transparency stick. You do not have to wire that up yourself.
 *
 * Each one also has a setter you can call at any time:
 *
 *     webview_window_set_frameless(&w, 0);
 *     webview_window_set_always_on_top(&w, 1);
 *     webview_window_set_opacity(&w, 200);
 *     webview_window_refresh_transparency(&w);   rebuild + re-apply
 *
 * The setters update w.window too, so it always reflects the real state.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

/* --------------------------------------------------------------- helpers */

static void webview_apply_topmost(struct webview *w) {
  SetWindowPos(w->priv.hwnd, w->window.always_on_top ? HWND_TOPMOST
                                                     : HWND_NOTOPMOST,
               0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void webview_apply_click_through(struct webview *w) {
  LONG_PTR ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  if (w->window.click_through) {
    /* WS_EX_TRANSPARENT needs WS_EX_LAYERED to actually pass clicks on. */
    ex |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
  } else {
    ex &= ~WS_EX_TRANSPARENT;
  }
  SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, ex);
}

static void webview_apply_taskbar(struct webview *w) {
  /* WS_EX_TOOLWINDOW keeps a window out of the taskbar and out of alt-tab,
     but the shell only notices while the window is hidden. */
  LONG_PTR ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  int visible = IsWindowVisible(w->priv.hwnd);
  LONG_PTR wanted =
      w->window.hide_from_taskbar ? (ex | WS_EX_TOOLWINDOW)
                                  : (ex & ~(LONG_PTR)WS_EX_TOOLWINDOW);
  if (wanted == ex) {
    return;
  }
  if (visible) {
    ShowWindow(w->priv.hwnd, SW_HIDE);
  }
  SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, wanted);
  if (visible) {
    ShowWindow(w->priv.hwnd, SW_SHOW);
  }
}

static void webview_apply_opacity(struct webview *w) {
  LONG_PTR ex;
  BYTE alpha = (BYTE)((w->window.opacity <= 0 || w->window.opacity > 255)
                          ? 255
                          : w->window.opacity);
  DWORD flags = LWA_ALPHA;
  if (alpha == 255 && !w->window.transparent) {
    return; /* nothing to do, and no reason to make the window layered */
  }
  if (alpha == 255 && webview_mode_is_glass(w->window.transparent) &&
      !webview_glass_wants_layered(w->window.transparent)) {
    return; /* glass runs without the layered prime, see wv_glass.inl */
  }
  ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
  /* A colour-keyed window keeps its key; the two flags combine. Glass sits
     on the same layered prime as ALPHA, so it keeps the key too. */
  if (w->window.transparent == WEBVIEW_WINDOW_COLORKEY ||
      w->window.transparent == WEBVIEW_WINDOW_ALPHA ||
      webview_glass_wants_layered(w->window.transparent)) {
    flags |= LWA_COLORKEY;
  }
  SetLayeredWindowAttributes(w->priv.hwnd,
                             RGB(webview_key_r, webview_key_g, webview_key_b),
                             alpha, flags);
}

static void webview_apply_center(struct webview *w) {
  RECT rect;
  int screen_w = GetSystemMetrics(SM_CXSCREEN);
  int screen_h = GetSystemMetrics(SM_CYSCREEN);
  if (!GetWindowRect(w->priv.hwnd, &rect)) {
    return;
  }
  SetWindowPos(w->priv.hwnd, NULL,
               (screen_w - (rect.right - rect.left)) / 2,
               (screen_h - (rect.bottom - rect.top)) / 2, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/* ----------------------------------------------------------- corners
 *
 *     w.window.corner_radius = 24;            or any time:
 *     webview_window_set_corner_radius(&w, 24);
 *
 * Pixels at 100% scaling; scaled with the window's DPI.
 *
 *    0   Windows decides: 8px on Windows 11 for a framed or glass window,
 *        square otherwise. The default.
 *   -1   square.
 *    4   Windows' small rounding    } drawn by DWM: antialiased, and the
 *    8   Windows' normal rounding   } shadow follows the curve.
 *  other a window region. Any radius, but its edge is 1-bit - a fine step
 *        pattern instead of a smooth curve - and Windows draws no shadow for
 *        a window with a region. A 1px border in the page along the same
 *        radius hides most of the steps; examples/10_glass does that.
 *
 * Windows 10 has no DWM rounding, so there every radius is a region.
 * Maximized and fullscreen windows are square, the way Windows does it. */
#define WEBVIEW_DWMWA_WINDOW_CORNER_PREFERENCE 33
#define WEBVIEW_DWMWCP_DEFAULT 0
#define WEBVIEW_DWMWCP_DONOTROUND 1
#define WEBVIEW_DWMWCP_ROUND 2
#define WEBVIEW_DWMWCP_ROUNDSMALL 3

static int webview_corner_region = 0; /* we set a window region */

static UINT webview_window_dpi(HWND hwnd) {
  typedef UINT(WINAPI * dpi_for_window_fn)(HWND);
  HMODULE user32 = GetModuleHandleA("user32.dll");
  dpi_for_window_fn dpi_for_window =
      (user32 != NULL) ? (dpi_for_window_fn)(void *)GetProcAddress(
                             user32, "GetDpiForWindow")
                       : NULL;
  UINT dpi = (dpi_for_window != NULL) ? dpi_for_window(hwnd) : 0;
  return (dpi != 0) ? dpi : 96;
}

static void webview_apply_corners(struct webview *w) {
  HWND hwnd;
  int r;
  int dwm_rounds;
  int pref;
  int region;
  RECT rect;

  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  hwnd = w->priv.hwnd;
  r = w->window.corner_radius;
  dwm_rounds = (webview_glass_build() >= 22000);

  region = (r > 0) && (!dwm_rounds || ((r != 4) && (r != 8)));
  if (r == 0) {
    pref = webview_mode_is_glass(webview_transparency_mode)
               ? WEBVIEW_DWMWCP_ROUND
               : WEBVIEW_DWMWCP_DEFAULT;
  } else if (r == 4) {
    pref = WEBVIEW_DWMWCP_ROUNDSMALL;
  } else if (r == 8) {
    pref = WEBVIEW_DWMWCP_ROUND;
  } else {
    pref = WEBVIEW_DWMWCP_DONOTROUND; /* square, or the region does it */
  }
  if (dwm_rounds) {
    webview_glass_attr(hwnd, WEBVIEW_DWMWA_WINDOW_CORNER_PREFERENCE, pref);
  }

  if (region && !IsZoomed(hwnd) && !w->priv.is_fullscreen &&
      GetWindowRect(hwnd, &rect)) {
    /* the ellipse is the diameter; +1 because the right and bottom edges
       of a region are exclusive */
    int d = MulDiv(r * 2, (int)webview_window_dpi(hwnd), 96);
    HRGN rgn = CreateRoundRectRgn(0, 0, rect.right - rect.left + 1,
                                  rect.bottom - rect.top + 1, d, d);
    if (rgn != NULL) {
      SetWindowRgn(hwnd, rgn, TRUE); /* the window owns it now */
      webview_corner_region = 1;
    }
  } else if (webview_corner_region) {
    SetWindowRgn(hwnd, NULL, TRUE);
    webview_corner_region = 0;
  }
}

WEBVIEW_API void webview_window_set_corner_radius(struct webview *w,
                                                  int radius) {
  if (w == NULL) { return; }
  w->window.corner_radius = radius;
  webview_apply_corners(w);
}

/* Force Windows to rebuild and repaint the non-client area.
 *
 * SWP_FRAMECHANGED tells Windows the frame changed, but it does not always
 * clean up what the old frame left behind: switch a frameless window back to
 * a framed one and you get a stale border and caption stuck on screen until
 * something makes the window genuinely change size. Resizing it by hand is
 * what usually clears it.
 *
 * So do that resize programmatically - one pixel smaller and straight back,
 * which is a real size change as far as Windows is concerned but far too
 * small and too quick to see - then invalidate the frame and the children
 * and paint it now rather than whenever the queue gets round to it.
 *
 * Call this BEFORE re-applying transparency: it uses SWP_FRAMECHANGED itself,
 * so it wipes the alpha channel exactly like any other frame change. */
static void webview_repaint_frame(struct webview *w, int erase) {
  RECT rect;
  int width;
  int height;
  UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED;

  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  if (!GetWindowRect(w->priv.hwnd, &rect)) {
    return;
  }
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;
  if ((width <= 1) || (height <= 1)) {
    return;
  }

  /* Drop any window region the old frame left behind, or the stale shape
     survives the resize. */
  SetWindowRgn(w->priv.hwnd, NULL, FALSE);

  /* A frame change is the other moment stale pixels appear. Arm the erase
     for it - and leave it armed a moment afterwards, because the repaints
     the style change sets off do not all arrive before this function
     returns. Disarming here caught only the synchronous ones, which is why
     one direction of the toggle still trailed. WM_TIMER disarms it.

     Only when there is something stale to clear, though. At startup the
     window is fresh, nothing has been drawn in the wrong place yet, and
     arming the erase just means the one pixel strip this resize uncovers
     gets painted black before the browser catches up - a thin black line
     down the right edge and along the bottom for a tenth of a second. */
  if (erase) {
    webview_erase_armed = 1;
    SetTimer(w->priv.hwnd, WEBVIEW_ERASE_TIMER, 350, NULL);
  }

  /* Write the style back over itself. The value does not change, but the
     write plus SWP_FRAMECHANGED is what the frameless toggle did, and a
     plain resize is not always enough to make DWM rebuild the surface. */
  SetWindowLongPtr(w->priv.hwnd, GWL_STYLE,
                   GetWindowLongPtr(w->priv.hwnd, GWL_STYLE));

  SetWindowPos(w->priv.hwnd, NULL, 0, 0, width - 1, height - 1, flags);
  SetWindowPos(w->priv.hwnd, NULL, 0, 0, width, height, flags);

  if (erase) {
    /* Wipe the whole window by hand, over the top of the browser child.
     *
     * Going framed -> frameless is the case ordinary erasing cannot reach.
     * The caption and border pixels were in the non client area; losing the
     * frame turns that same area into client area, which the browser child
     * now covers. WS_CLIPCHILDREN then stops the parent painting there, and
     * the child does not clear it either - the page is transparent in that
     * strip, so nothing is drawn and whatever was in the surface stays. The
     * result is a ghost frame around a frameless window, and no amount of
     * invalidating the parent touches it.
     *
     * GetDCEx with DCX_WINDOW and no DCX_CLIPCHILDREN hands back a DC for
     * the entire window that ignores the child, so the fill actually lands.
     * It is black, which is zero alpha, which is transparent - so this reads
     * as a single transparent frame, not a black flash, and the browser
     * paints over it immediately afterwards. */
    HDC dc = GetDCEx(w->priv.hwnd, NULL, DCX_WINDOW | DCX_CACHE);
    if (dc != NULL) {
      RECT full;
      full.left = 0;
      full.top = 0;
      full.right = width;
      full.bottom = height;
      FillRect(dc, &full, (HBRUSH)GetStockObject(BLACK_BRUSH));
      ReleaseDC(w->priv.hwnd, dc);
    }
  }

  RedrawWindow(w->priv.hwnd, NULL, NULL,
               RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN |
                   RDW_UPDATENOW);
}

/* Re-apply the whole transparency stack.
 *
 * Anything that changes the window frame - SWP_FRAMECHANGED, going
 * fullscreen, DWM restarting - makes Windows rebuild the window's surface,
 * and the rebuilt one does not inherit the blur-behind alpha region, the
 * DWMSBT_NONE backdrop attribute or the layered flag. The window is left with
 * no alpha channel at all, so every pixel the page leaves transparent
 * composites onto nothing and comes out BLACK. It stays black until
 * something puts the alpha channel back, which is what this does.
 *
 * Cheap enough to call after any style change; does nothing when
 * transparency was never switched on. */
static void webview_refresh_transparency(struct webview *w) {
  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  if (w->window.transparent) {
    webview_window_transparency(w, w->window.transparent);
  }
  if (w->window.click_through) {
    webview_apply_click_through(w);
  }
  webview_apply_opacity(w);
  /* webview_repaint_frame() drops any window region, a custom radius too */
  if (w->window.corner_radius != 0) {
    webview_apply_corners(w);
  }
}

/* ------------------------------------------------------------- the setters */

WEBVIEW_API void webview_window_set_frameless(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->window.frameless = on;
  webview_set_frameless(w, on);
  /* clear the frame the old style left on screen, THEN put the alpha channel
     back - the repaint is itself a frame change and would wipe it again */
  webview_repaint_frame(w, 1); /* clear what the old frame left behind */
  webview_refresh_transparency(w);
}

/* The window half of fullscreen, shared by the setter below and by pages
   asking for it themselves. */
static void webview_fullscreen_apply(struct webview *w, int on) {
  w->window.fullscreen = on ? 1 : 0;
  webview_set_fullscreen(w, on);
  webview_repaint_frame(w, 1);
  webview_refresh_transparency(w);
}

WEBVIEW_API void webview_window_set_fullscreen(struct webview *w, int on) {
  if (w == NULL) { return; }
  /* Your call wins from here on. If a page had made the window fullscreen,
     the window is now yours: the page leaving fullscreen later will not
     take it out again. */
  w->priv.fullscreen_by_page = 0;
  if (!on) {
    /* and leaving fullscreen takes the page's element out with it, or a
       video would stay stretched over a normal sized window */
    if (w->priv.page_fullscreen && webview_webview2_enabled &&
        (w->priv.webview2 != NULL) && (w->priv.webview2->webview != NULL)) {
      WebView2ExecuteOn(w->priv.webview2->webview, w->priv.hwnd,
                        WEBVIEW_JS_EXIT_FULLSCREEN, NULL, NULL);
    }
    webview_guests_exit_fullscreen(w);
  }
  webview_fullscreen_apply(w, on);
}

/* ------------------------------------------------------- HTML fullscreen
 *
 * element.requestFullscreen() in a page - a video's fullscreen button, a
 * game, a presentation - in the main page or in a <webview> guest. WebView2
 * only grows the element to fill the browser; taking the WINDOW fullscreen
 * is our job. This is what a browser tab does:
 *
 *   page enters fullscreen   the window goes fullscreen, unless it already
 *                            was (then nothing to do)
 *   page leaves fullscreen   the window comes back exactly as it was -
 *   (Esc, exitFullscreen(),  size, position, maximized - but only if it was
 *    the page navigating)    the page that made it fullscreen. A window you
 *                            made fullscreen yourself stays fullscreen.
 *
 * w.browser.html_fullscreen = WEBVIEW_OFF turns it off: the element then
 * fills the window and the window stays as it is (WebView2's own default).
 * It is read when a page asks, so you can change it at any time. Not on the
 * MSHTML/IE fallback, which has no such event.
 * -------------------------------------------------------------------------*/
static int webview_html_fullscreen_allowed(struct webview *w) {
  return (w != NULL) && (w->browser.html_fullscreen != WEBVIEW_OFF);
}

static void webview_html_fullscreen_changed(struct webview *w) {
  int wanted;
  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  wanted = webview_html_fullscreen_allowed(w) &&
           (w->priv.page_fullscreen || webview_guests_any_fullscreen(w));
  if (wanted && !w->priv.is_fullscreen) {
    w->priv.fullscreen_by_page = 1;
    webview_fullscreen_apply(w, 1);
  } else if (!wanted && w->priv.fullscreen_by_page) {
    w->priv.fullscreen_by_page = 0;
    webview_fullscreen_apply(w, 0);
  }
}

WEBVIEW_API void webview_window_set_always_on_top(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->window.always_on_top = on;
  webview_apply_topmost(w);
}

WEBVIEW_API void webview_window_set_click_through(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->window.click_through = on;
  webview_apply_click_through(w);
}

WEBVIEW_API void webview_window_set_taskbar(struct webview *w, int shown) {
  if (w == NULL) { return; }
  w->window.hide_from_taskbar = shown ? 0 : 1;
  webview_apply_taskbar(w);
}

/* 0..255. 255 is fully opaque. This is uniform window opacity, which is a
   different thing from w.window.transparent: it fades the whole window,
   including anything the page painted solid. */
WEBVIEW_API void webview_window_set_opacity(struct webview *w, int opacity) {
  if (w == NULL) { return; }
  w->window.opacity = opacity;
  webview_apply_opacity(w);
}

WEBVIEW_API void webview_window_set_transparent(struct webview *w, int mode) {
  if (w == NULL) { return; }
  w->window.transparent = mode;
  /* A frameless window is built differently in the glass modes (frame kept
     and hidden, see wv_glass.inl). Rebuild it when that changes - frame
     first, then the transparency, the same order as the frameless toggle. */
  if (w->window.frameless &&
      (webview_mode_is_glass(mode) != w->priv.hidden_frame)) {
    webview_set_frameless(w, 1);
    webview_repaint_frame(w, 1);
  }
  webview_window_transparency(w, mode);
}

/* Put the whole transparency stack back the robust way: rebuild the window
   surface first, then re-apply - the same sequence the frameless toggle
   uses. For after you changed something DWM only reads when the surface is
   built, such as webview_glass_tuning() or webview_set_glass_dark(). */
WEBVIEW_API void webview_window_refresh_transparency(struct webview *w) {
  if ((w == NULL) || (w->priv.hwnd == NULL)) { return; }
  webview_repaint_frame(w, 1);
  webview_refresh_transparency(w);
}

WEBVIEW_API void webview_window_center(struct webview *w) {
  if (w == NULL) { return; }
  webview_apply_center(w);
}

/* How many times to re-apply transparency after the page comes up, and how
   far apart. DWM can take a moment to settle after the browser's first frame,
   and an application that lands too early is silently ignored - the window
   stays black. Re-applying costs nothing and is invisible, so do it a few
   times and stop. */
#define WEBVIEW_TRANSPARENCY_TRIES 6
#define WEBVIEW_TRANSPARENCY_DELAY 120

static int webview_transparency_tries = 0;

/* Called from WM_TIMER. Returns 1 while it still wants to be called again. */
static int webview_retry_transparency(struct webview *w) {
  if ((w == NULL) || (w->priv.hwnd == NULL) || !w->window.transparent) {
    return 0;
  }
  webview_repaint_frame(w, 0); /* nothing stale yet - do not paint */
  webview_refresh_transparency(w);
  webview_transparency_tries++;
  return (webview_transparency_tries < WEBVIEW_TRANSPARENCY_TRIES) ? 1 : 0;
}

/* ------------------------------------------------------------ apply it all */

/* Everything that does not need the browser. Runs at the end of
   webview_init(), before the window is shown, so there is no flicker. */
static void webview_apply_window_options(struct webview *w) {
  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  if (w->window.frameless) {
    webview_set_frameless(w, 1);
  }
  if (w->window.always_on_top) {
    webview_apply_topmost(w);
  }
  if (w->window.hide_from_taskbar) {
    webview_apply_taskbar(w);
  }
  if (w->window.click_through) {
    webview_apply_click_through(w);
  }
  if (w->window.center) {
    webview_apply_center(w);
  }
  if (w->window.maximized) {
    ShowWindow(w->priv.hwnd, SW_MAXIMIZE);
  }
  if (w->window.fullscreen) {
    webview_set_fullscreen(w, 1);
  }
  webview_apply_opacity(w);
  if (w->window.corner_radius != 0) {
    webview_apply_corners(w);
  }
}

/* The browser-side half. Called once the page reports that it is up, which
   is the timing transparency needs - see TRANSPARENCY.md. */
static void webview_apply_page_options(struct webview *w) {
  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  /* The page is up. Everything that must not run before that point keys off
     this - see webview_erase_transparent. */
  w->window.webview_loaded = 1;
  if (w->window.transparent) {
    /* The frame nudge first, then the transparency - the same order the
       frameless toggle uses, and that order matters.
       DWM does not always honour a blur-behind alpha region on a window
       whose surface it has already settled on: the call succeeds, the alpha
       channel never appears, and the window comes up solid black (black
       being what WM_ERASEBKGND paints when it expects to be transparent).
       Toggling the frame used to fix it by hand, because the frame change
       rebuilt the surface and transparency was re-applied afterwards. So do
       the rebuild here too. It is a one pixel resize; nobody sees it. */
    webview_repaint_frame(w, 0); /* nothing stale yet - do not paint */
    webview_refresh_transparency(w);
    /* ...and again shortly, a few times. See webview_retry_transparency. */
    webview_transparency_tries = 0;
    SetTimer(w->priv.hwnd, WEBVIEW_TRANSPARENCY_TIMER,
             WEBVIEW_TRANSPARENCY_DELAY, NULL);
  }
}
