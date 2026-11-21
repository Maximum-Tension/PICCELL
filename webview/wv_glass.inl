/* ---------------------------------------------------------------------------
 * wv_glass.inl - a blurred, see-through window ("glass")
 *
 *     w.window.transparent = WEBVIEW_WINDOW_GLASS;
 *     w.window.frameless   = 1;                      optional, see below
 *     webview_prefer_transparent(1);                  before init, as always
 *     webview_init(&w);
 *
 *     html, body { background: transparent; }        plus a light tint
 *
 * Four modes, all switchable live with webview_window_set_transparent():
 *
 *   WEBVIEW_WINDOW_GLASS     frosted glass: whatever is behind the window,
 *                            blurred. Windows 11 22H2+ uses Desktop Acrylic
 *                            (DWMSBT_TRANSIENTWINDOW); older Windows 10/11
 *                            falls back to the accent-policy blur.
 *   WEBVIEW_WINDOW_MICA      Windows 11 Mica. Tinted by the WALLPAPER, not by
 *                            the windows behind it - it is not see-through.
 *   WEBVIEW_WINDOW_MICA_ALT  Mica Alt (DWMSBT_TABBEDWINDOW), a deeper tint.
 *   WEBVIEW_WINDOW_BLUR      the undocumented accent-policy blur, on any
 *                            Windows 10/11. Keeps blurring when the window is
 *                            not focused; can lag while dragging on 11 22H2+.
 *
 * The blur happens in DWM, outside the browser. The page cannot blur the
 * desktop itself: backdrop-filter only ever sees pixels the page painted, and
 * in a transparent window the area behind an element is empty. So Windows
 * does the blur and the page does the tint, the borders and the highlights.
 *
 * How it is built, on top of what WEBVIEW_WINDOW_ALPHA already does:
 *
 *  1. The alpha stack: an empty blur-behind region and WebView2's
 *     background at alpha 0, so DWM can see through the page. Unlike
 *     WEBVIEW_WINDOW_ALPHA, no layered prime by default: every material
 *     works without it, the accent blur goes black with it, and Windows 11
 *     gives layered windows no rounded corners and no shadow. The
 *     accent-policy route skips the blur-behind call too, because stacking
 *     the two changes what DWM composites (see webview_set_transparent).
 *  2. A material behind the window: DWMWA_SYSTEMBACKDROP_TYPE on 22621+, the
 *     older undocumented DWMWA_MICA_EFFECT on 22000, the accent policy below
 *     that.
 *  3. DWMWA_USE_IMMERSIVE_DARK_MODE picks the dark or light variant of the
 *     material, and DWMWA_WINDOW_CORNER_PREFERENCE rounds the corners.
 *
 * Two things other people found the hard way, built in:
 *
 *  - DWM silently ignores the system backdrop on a plain WS_POPUP window.
 *    It only puts material behind windows it considers framed, which means
 *    WS_CAPTION. So in a glass mode, frameless keeps WS_CAPTION and
 *    WS_THICKFRAME and hides the frame instead, by answering WM_NCCALCSIZE
 *    with "the client area is the whole window". DWM keeps the shadow, the
 *    rounded corners and the 1px border, and snap still works.
 *  - DWM swaps the material for a flat colour while the window is inactive.
 *    Answering WM_NCACTIVATE as if always active keeps it on screen. That
 *    changes how DWM paints the window, not which window has focus.
 *
 * The user can switch all of it off: with Transparency effects off in
 * Windows settings, energy saver on, or over Remote Desktop, DWM paints a flat
 * colour instead of any material, for every app. webview_glass_blocked() says
 * so, and the page hears about it when it changes - see below.
 *
 * What C cannot tell you: DWM reports whether it ACCEPTED an attribute, not
 * whether material ended up on screen. webview_glass_info() says which API
 * was used and what it returned; your eyes do the rest. If the window comes
 * up clear but not blurred, try webview_glass_tuning() - see below.
 *
 * Included by webview.h after wv_transparent.inl. Not meant to be compiled
 * on its own.
 * -------------------------------------------------------------------------*/

/* DWM attributes this file uses, by number, so older headers do not matter. */
#define WV_DWMWA_USE_IMMERSIVE_DARK_MODE_OLD 19 /* Windows 10 before 20H1 */
#define WV_DWMWA_USE_IMMERSIVE_DARK_MODE 20
#define WV_DWMWA_WINDOW_CORNER_PREFERENCE 33
#define WV_DWMWA_SYSTEMBACKDROP_TYPE 38
#define WV_DWMWA_MICA_EFFECT 1029 /* 22000 only, undocumented */

#define WV_DWMSBT_AUTO 0
#define WV_DWMSBT_NONE 1
#define WV_DWMSBT_MAINWINDOW 2      /* Mica */
#define WV_DWMSBT_TRANSIENTWINDOW 3 /* Desktop Acrylic */
#define WV_DWMSBT_TABBEDWINDOW 4    /* Mica Alt */

#define WV_DWMWCP_DEFAULT 0
#define WV_DWMWCP_ROUND 2

#define WV_ACCENT_DISABLED 0
#define WV_ACCENT_ENABLE_BLURBEHIND 3

/* Which API ended up drawing the material. */
enum {
  WV_GLASS_API_NONE = 0,
  WV_GLASS_API_SYSTEM = 1, /* DWMWA_SYSTEMBACKDROP_TYPE, 22621+  */
  WV_GLASS_API_MICA = 2,   /* DWMWA_MICA_EFFECT, 22000         */
  WV_GLASS_API_ACCENT = 3  /* SetWindowCompositionAttribute    */
};

/* --------------------------------------------------------------- tuning */

/* The defaults are what examples/10_glass's self-test found working on a
   Windows 11 25H2 machine: every material, hidden frame or not. Each one is
   still a switch, because DWM's behaviour varies between builds and nothing
   reports which way it went - run the self-test on a machine that differs. */
static int webview_glass_layered = 0;     /* the ALPHA stack's layered prime */
static int webview_glass_blur_behind = 1; /* empty blur-behind region: alpha  */
static int webview_glass_extend = 0;      /* also DwmExtendFrameIntoClientArea */
static int webview_glass_keep_active = 1; /* material stays when unfocused    */
static int webview_glass_dark = 1;        /* dark variant of the material     */

/* -1 leaves a switch as it is. Takes effect the next time a glass mode is
   applied - webview_window_refresh_transparency() applies it now.

     layered      0: (default) no layered prime, like
                     WEBVIEW_WINDOW_ALPHA_PLAIN.
                  1: the layered prime WEBVIEW_WINDOW_ALPHA uses, in case a
                     machine will not grant alpha without it. Never applied
                     to the accent blur, which goes black with it.
     blur_behind  1: the empty blur-behind region WEBVIEW_WINDOW_ALPHA uses
                     to get an alpha channel.
                  0: leave it out, for when extend_frame provides the alpha.
     extend_frame 1: also extend the frame over the whole client area, the
                     textbook route to "material behind the client area".
                     Off by default: on top of blur-behind it can change what
                     DWM composites, and on a framed window it makes DWM draw
                     the caption buttons over your page.
     keep_active  1: answer WM_NCACTIVATE as if always active, so the material
                     does not go flat when another window has focus. */
WEBVIEW_API void webview_glass_tuning(int layered, int blur_behind,
                                      int extend_frame, int keep_active) {
  if (layered >= 0) {
    webview_glass_layered = layered ? 1 : 0;
  }
  if (blur_behind >= 0) {
    webview_glass_blur_behind = blur_behind ? 1 : 0;
  }
  if (extend_frame >= 0) {
    webview_glass_extend = extend_frame ? 1 : 0;
  }
  if (keep_active >= 0) {
    webview_glass_keep_active = keep_active ? 1 : 0;
  }
}

/* Dark or light variant of the material. The page's own tint should follow
   suit. Takes effect the next time a glass mode is applied. */
WEBVIEW_API void webview_set_glass_dark(int dark) {
  webview_glass_dark = dark ? 1 : 0;
}

/* ---------------------------------------------------------------- state */

static int webview_glass_api = WV_GLASS_API_NONE;
static int webview_glass_accent_on = 0;    /* we turned the accent on      */
static int webview_glass_backdrop_set = 0; /* we set a backdrop type        */
static int webview_glass_mica_set = 0;     /* we set DWMWA_MICA_EFFECT      */
static int webview_glass_extended = 0;     /* we extended the frame         */
static int webview_glass_corners_set = 0;  /* we asked for rounded corners  */
static char webview_glass_desc[384] = "no glass mode applied yet";

static int webview_mode_is_glass(int mode) {
  return (mode == WEBVIEW_WINDOW_GLASS) || (mode == WEBVIEW_WINDOW_MICA) ||
         (mode == WEBVIEW_WINDOW_MICA_ALT) || (mode == WEBVIEW_WINDOW_BLUR);
}

static int webview_glass_build(void);

/* Which API draws a glass mode on this Windows. The accent blur is the only
   option before Windows 11, and the only see-through one on 22000, where
   acrylic through the accent policy lags badly while dragging. */
static int webview_glass_api_for(int mode) {
  int build = webview_glass_build();
  if ((mode == WEBVIEW_WINDOW_BLUR) || (build < 22000)) {
    return WV_GLASS_API_ACCENT;
  }
  if (build >= 22621) {
    return WV_GLASS_API_SYSTEM;
  }
  return (mode == WEBVIEW_WINDOW_GLASS) ? WV_GLASS_API_ACCENT
                                        : WV_GLASS_API_MICA;
}

/* Does this mode get the layered prime? Only when switched on, and never on
   the accent route: the self-test found the accent blur black with it and
   blurred without it. */
static int webview_glass_wants_layered(int mode) {
  return webview_mode_is_glass(mode) && webview_glass_layered &&
         (webview_glass_api_for(mode) != WV_GLASS_API_ACCENT);
}

/* The wndproc asks this on every WM_NCACTIVATE. */
static int webview_glass_keep_active_now(struct webview *w) {
  return webview_glass_keep_active && (w != NULL) && w->priv.transparent &&
         webview_mode_is_glass(webview_transparency_mode);
}

/* One line on what the last glass call did: the API, its HRESULT and the
   switches. Good for a status bar or the log. */
WEBVIEW_API const char *webview_glass_info(void) { return webview_glass_desc; }

/* Why Windows will not draw ANY material right now, or NULL if it will.
 *
 * These are the user's own settings. With any of them, DWM switches every
 * material off for every app on the machine and paints a flat colour instead:
 * Mica becomes #202020 (dark), acrylic a flat grey, the accent blur black.
 * No API overrides them, and nothing should - it is the user's choice. What
 * an app can do is notice, and look intentional without the glass, e.g. by
 * painting a more solid background. Read fresh on every call. */
WEBVIEW_API const char *webview_glass_blocked(void) {
  DWORD enabled = 1;
  DWORD size = sizeof(enabled);
  SYSTEM_POWER_STATUS power;
  if ((webview_reg_read(HKEY_CURRENT_USER,
                        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\"
                        "Personalize",
                        "EnableTransparency", REG_DWORD, &enabled,
                        &size) == ERROR_SUCCESS) &&
      (enabled == 0)) {
    return "Transparency effects are off in Windows settings";
  }
  ZeroMemory(&power, sizeof(power));
  /* the 4th byte is SystemStatusFlag (Reserved1 in older headers) */
  if (GetSystemPowerStatus(&power) && ((BYTE *)&power)[3]) {
    return "Energy saver is on";
  }
  if (GetSystemMetrics(SM_REMOTESESSION)) {
    return "this is a Remote Desktop session";
  }
  return NULL;
}

/* Tell the page whether glass can show, as the "glass" event:
     window.webview.on("glass", function (e) { e.blocked })   null = it can */
static void webview_glass_announce(struct webview *w) {
  const char *why = webview_glass_blocked();
  char json[192];
  if (why != NULL) {
    snprintf(json, sizeof(json), "{\"blocked\":\"%s\"}", why);
  } else {
    snprintf(json, sizeof(json), "{\"blocked\":null}");
  }
  webview_emit(w, "glass", json);
}

/* WM_SETTINGCHANGE / WM_POWERBROADCAST: the user may just have switched
   transparency effects or energy saver. Put the stack back and tell the
   page. Only while a glass mode is on; colour-scheme changes arrive as
   "ImmersiveColorSet", power changes with no string at all. */
static void webview_glass_settings_changed(struct webview *w, UINT msg,
                                           LPARAM lParam) {
  if ((w == NULL) || (w->priv.hwnd == NULL) ||
      !webview_mode_is_glass(webview_transparency_mode) ||
      !w->window.webview_loaded) {
    return;
  }
  if (msg == WM_SETTINGCHANGE) {
    /* the string is wide for a Unicode window, ANSI otherwise */
    if ((lParam == 0) ||
        (IsWindowUnicode(w->priv.hwnd)
             ? (lstrcmpiW((LPCWSTR)lParam, L"ImmersiveColorSet") != 0)
             : (lstrcmpiA((LPCSTR)lParam, "ImmersiveColorSet") != 0))) {
      return;
    }
  }
  webview_refresh_transparency(w);
  webview_glass_announce(w);
}

static int webview_glass_build(void) {
  static int build = -1;
  if (build < 0) {
    webview_windows_version(NULL, NULL, &build);
  }
  return build;
}

/* ------------------------------------------------------------- DWM calls */

typedef HRESULT(WINAPI *webview_dwm_set_attribute_fn)(HWND, DWORD, LPCVOID,
                                                      DWORD);
typedef struct {
  int cxLeftWidth;
  int cxRightWidth;
  int cyTopHeight;
  int cyBottomHeight;
} WV_MARGINS;
typedef HRESULT(WINAPI *webview_dwm_extend_fn)(HWND, const WV_MARGINS *);
typedef HRESULT(WINAPI *webview_dwm_blur_fn)(HWND, const DWM_BLURBEHIND *);

/* dwmapi.dll stays loaded: the window keeps using what we asked it for. */
static HMODULE webview_glass_dwm(void) {
  static HMODULE dwm = NULL;
  if (dwm == NULL) {
    dwm = LoadLibraryA("dwmapi.dll");
  }
  return dwm;
}

static HRESULT webview_glass_attr(HWND hwnd, DWORD attr, int value) {
  HMODULE dwm = webview_glass_dwm();
  webview_dwm_set_attribute_fn set =
      (dwm != NULL) ? (webview_dwm_set_attribute_fn)(void *)GetProcAddress(
                          dwm, "DwmSetWindowAttribute")
                    : NULL;
  if (set == NULL) {
    return E_NOTIMPL;
  }
  return set(hwnd, attr, &value, sizeof(value));
}

static HRESULT webview_glass_extend_frame(HWND hwnd, int on) {
  HMODULE dwm = webview_glass_dwm();
  WV_MARGINS margins;
  webview_dwm_extend_fn extend =
      (dwm != NULL) ? (webview_dwm_extend_fn)(void *)GetProcAddress(
                          dwm, "DwmExtendFrameIntoClientArea")
                    : NULL;
  if (extend == NULL) {
    return E_NOTIMPL;
  }
  /* -1 everywhere: "sheet of glass", the frame covers the client area */
  margins.cxLeftWidth = on ? -1 : 0;
  margins.cxRightWidth = on ? -1 : 0;
  margins.cyTopHeight = on ? -1 : 0;
  margins.cyBottomHeight = on ? -1 : 0;
  return extend(hwnd, &margins);
}

/* Turn the blur-behind alpha region off again. Harmless if it was off. */
static void webview_glass_blur_behind_off(HWND hwnd) {
  HMODULE dwm = webview_glass_dwm();
  DWM_BLURBEHIND bb;
  webview_dwm_blur_fn blur =
      (dwm != NULL) ? (webview_dwm_blur_fn)(void *)GetProcAddress(
                          dwm, "DwmEnableBlurBehindWindow")
                    : NULL;
  if (blur == NULL) {
    return;
  }
  ZeroMemory(&bb, sizeof(bb));
  bb.dwFlags = DWM_BB_ENABLE;
  bb.fEnable = FALSE;
  blur(hwnd, &bb);
}

/* The undocumented accent policy - the only way to ask for a blur before
   Windows 11 22H2. Returns 0 on success. */
static int webview_glass_accent(HWND hwnd, int state) {
  typedef struct {
    int AccentState;
    DWORD AccentFlags;
    DWORD GradientColor; /* AABBGGRR, ignored by the plain blur */
    DWORD AnimationId;
  } WV_GLASS_ACCENT;
  typedef struct {
    DWORD Attrib; /* 19 = WCA_ACCENT_POLICY */
    PVOID pvData;
    SIZE_T cbData;
  } WV_GLASS_COMPOSITION;
  typedef BOOL(WINAPI * set_composition_fn)(HWND, WV_GLASS_COMPOSITION *);
  HMODULE user32 = GetModuleHandleA("user32.dll");
  set_composition_fn set =
      (user32 != NULL) ? (set_composition_fn)(void *)GetProcAddress(
                             user32, "SetWindowCompositionAttribute")
                       : NULL;
  WV_GLASS_ACCENT accent;
  WV_GLASS_COMPOSITION data;
  if (set == NULL) {
    return -1;
  }
  ZeroMemory(&accent, sizeof(accent));
  accent.AccentState = state;
  data.Attrib = 19;
  data.pvData = &accent;
  data.cbData = sizeof(accent);
  return set(hwnd, &data) ? 0 : -1;
}

static void webview_glass_dark_mode(HWND hwnd, int dark) {
  if (FAILED(webview_glass_attr(hwnd, WV_DWMWA_USE_IMMERSIVE_DARK_MODE,
                                dark))) {
    webview_glass_attr(hwnd, WV_DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, dark);
  }
}

/* Layered on or off. Off only when nothing else needs it: click-through and
   uniform opacity both depend on WS_EX_LAYERED. */
static void webview_glass_set_layered(struct webview *w, int on) {
  LONG_PTR ex;
  if (on) {
    webview_prime_layered(w);
    return;
  }
  if (w->window.click_through ||
      ((w->window.opacity > 0) && (w->window.opacity < 255))) {
    return;
  }
  ex = GetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE);
  if (ex & WS_EX_LAYERED) {
    SetWindowLongPtr(w->priv.hwnd, GWL_EXSTYLE, ex & ~(LONG_PTR)WS_EX_LAYERED);
  }
}

/* ---------------------------------------------------- leaving glass behind */

/* Undo whatever a glass mode set, so the next mode starts clean. Called by
   webview_window_transparency() for every non-glass mode, and costs nothing
   when glass was never on. */
static void webview_glass_clear(struct webview *w, int mode) {
  HWND hwnd;
  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  hwnd = w->priv.hwnd;
  if (webview_glass_accent_on) {
    webview_glass_accent(hwnd, WV_ACCENT_DISABLED);
    webview_glass_accent_on = 0;
  }
  if (webview_glass_backdrop_set) {
    /* WEBVIEW_WINDOW_ALPHA puts DWMSBT_NONE over this straight afterwards */
    webview_glass_attr(hwnd, WV_DWMWA_SYSTEMBACKDROP_TYPE, WV_DWMSBT_AUTO);
    webview_glass_backdrop_set = 0;
  }
  if (webview_glass_mica_set) {
    webview_glass_attr(hwnd, WV_DWMWA_MICA_EFFECT, 0);
    webview_glass_mica_set = 0;
  }
  if (webview_glass_extended) {
    webview_glass_extend_frame(hwnd, 0);
    webview_glass_extended = 0;
  }
  if (webview_glass_corners_set) {
    webview_glass_attr(hwnd, WV_DWMWA_WINDOW_CORNER_PREFERENCE,
                       WV_DWMWCP_DEFAULT);
    webview_glass_corners_set = 0;
    if (w->window.corner_radius != 0) {
      webview_apply_corners(w); /* a radius asked for outlives the glass */
    }
  }
  if (webview_glass_api != WV_GLASS_API_NONE) {
    webview_glass_api = WV_GLASS_API_NONE;
    snprintf(webview_glass_desc, sizeof(webview_glass_desc),
             "no material (transparency mode %d)", mode);
  }
}

/* --------------------------------------------------------------- THE call */

static int webview_glass_apply(struct webview *w, int mode) {
  HWND hwnd;
  int build = webview_glass_build();
  int api;
  int type = WV_DWMSBT_TRANSIENTWINDOW;
  const char *material = "Desktop Acrylic";
  HRESULT hr = S_OK;
  int result = 0;

  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return -1;
  }
  hwnd = w->priv.hwnd;

  api = webview_glass_api_for(mode);
  if (mode == WEBVIEW_WINDOW_MICA) {
    type = WV_DWMSBT_MAINWINDOW;
    material = "Mica";
  } else if (mode == WEBVIEW_WINDOW_MICA_ALT) {
    type = WV_DWMSBT_TABBEDWINDOW;
    material = "Mica Alt";
  }

  if (api == WV_GLASS_API_ACCENT) {
    /* Accent on its own, the same way webview_set_acrylic does it: no
       blur-behind region, no system backdrop, no extended frame. On 22621+
       a framed window gets a system backdrop by default, which would sit
       between the blur and the page - ask for none. */
    if (build >= 22621) {
      webview_glass_attr(hwnd, WV_DWMWA_SYSTEMBACKDROP_TYPE, WV_DWMSBT_NONE);
      webview_glass_backdrop_set = 1;
    }
    if (webview_glass_mica_set) {
      webview_glass_attr(hwnd, WV_DWMWA_MICA_EFFECT, 0);
      webview_glass_mica_set = 0;
    }
    if (webview_glass_extended) {
      webview_glass_extend_frame(hwnd, 0);
      webview_glass_extended = 0;
    }
    webview_glass_blur_behind_off(hwnd);
    webview_glass_set_layered(w, webview_glass_wants_layered(mode));
    webview_glass_dark_mode(hwnd, webview_glass_dark); /* caption colour */
    if (webview_glass_accent(hwnd, WV_ACCENT_ENABLE_BLURBEHIND) == 0) {
      webview_glass_accent_on = 1;
    } else {
      webview_print_log("glass: SetWindowCompositionAttribute failed");
      hr = E_FAIL;
      result = -1;
    }
    material = "accent blur";
    w->priv.transparent = 1;
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)NULL);
    if (!webview_webview2_enabled ||
        (WebView2SetTransparent(w->priv.webview2, 1) != 0)) {
      result = -1;
    }
  } else {
    /* the accent and the system backdrop are two different answers to the
       same question - only one at a time */
    if (webview_glass_accent_on) {
      webview_glass_accent(hwnd, WV_ACCENT_DISABLED);
      webview_glass_accent_on = 0;
    }
    /* 1. the alpha stack, exactly as WEBVIEW_WINDOW_ALPHA builds it, minus
          its DWMSBT_NONE - we want a backdrop here */
    webview_glass_set_layered(w, webview_glass_wants_layered(mode));
    if (webview_glass_blur_behind) {
      if (webview_set_transparent(w, 1) != 0) {
        result = -1;
      }
    } else {
      /* everything webview_set_transparent does except the blur-behind */
      webview_glass_blur_behind_off(hwnd);
      w->priv.transparent = 1;
      SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)NULL);
      if (!webview_webview2_enabled ||
          (WebView2SetTransparent(w->priv.webview2, 1) != 0)) {
        result = -1;
      }
      InvalidateRect(hwnd, NULL, TRUE);
    }
    /* 2. the material */
    webview_glass_dark_mode(hwnd, webview_glass_dark);
    if (api == WV_GLASS_API_SYSTEM) {
      hr = webview_glass_attr(hwnd, WV_DWMWA_SYSTEMBACKDROP_TYPE, type);
      webview_glass_backdrop_set = 1;
    } else {
      hr = webview_glass_attr(hwnd, WV_DWMWA_MICA_EFFECT, 1);
      webview_glass_mica_set = 1;
      material = "Mica (22000)";
    }
    if (FAILED(hr)) {
      result = -1;
    }
    /* 3. optionally, the textbook frame extension */
    if (webview_glass_extend) {
      webview_glass_extend_frame(hwnd, 1);
      webview_glass_extended = 1;
    } else if (webview_glass_extended) {
      webview_glass_extend_frame(hwnd, 0);
      webview_glass_extended = 0;
    }
  }

  /* Corners: Windows' own rounding by default - it rounds framed windows
     already, and this covers the hidden frame - or whatever radius
     webview_window_set_corner_radius() asked for. */
  webview_apply_corners(w);
  webview_glass_corners_set = (build >= 22000) ? 1 : 0;

  /* Paint the window as active once, in case it lost focus before the page
     came up - otherwise it would wait for the next activation to show the
     material. Our WM_NCACTIVATE handler turns this into "active". */
  if (webview_glass_keep_active && (GetForegroundWindow() != hwnd)) {
    SendMessage(hwnd, WM_NCACTIVATE, TRUE, 0);
  }

  webview_glass_api = api;
  snprintf(webview_glass_desc, sizeof(webview_glass_desc),
           "%s via %s -> 0x%08lX | build %d | %s | layered %s, blur-behind %s, "
           "extend %s, keep-active %s",
           material,
           (api == WV_GLASS_API_SYSTEM) ? "DWMWA_SYSTEMBACKDROP_TYPE"
           : (api == WV_GLASS_API_MICA) ? "DWMWA_MICA_EFFECT"
                                        : "SetWindowCompositionAttribute",
           (unsigned long)hr, build, webview_glass_dark ? "dark" : "light",
           webview_glass_wants_layered(mode) ? "on" : "off",
           webview_glass_blur_behind ? "on" : "off",
           webview_glass_extend ? "on" : "off",
           webview_glass_keep_active ? "on" : "off");
  {
    const char *why = webview_glass_blocked();
    size_t used = strlen(webview_glass_desc);
    if ((why != NULL) && (used + 1 < sizeof(webview_glass_desc))) {
      snprintf(webview_glass_desc + used, sizeof(webview_glass_desc) - used,
               " | %s: Windows shows a flat colour instead", why);
    }
  }
  webview_print_log(webview_glass_desc);
  return result;
}

/* ----------------------------------------------------- the hidden frame
 *
 * WM_NCCALCSIZE for a frameless window in a glass mode: the client area is
 * the whole window, so no caption or border is drawn, while the styles that
 * make DWM treat it as a framed window stay. See the top of this file. */
static int webview_glass_nccalcsize(struct webview *w, HWND hwnd,
                                    WPARAM wParam, LPARAM lParam,
                                    LRESULT *result) {
  if ((w == NULL) || !w->priv.hidden_frame || !wParam ||
      w->priv.is_fullscreen) {
    return 0;
  }
  if (IsZoomed(hwnd)) {
    /* A maximized window is placed one frame thickness beyond the monitor
       on every side, so that its borders are off screen. With no frame the
       page would hang over the edges by that much - pull it back in. */
    typedef UINT(WINAPI * dpi_for_window_fn)(HWND);
    typedef int(WINAPI * metrics_for_dpi_fn)(int, UINT);
    HMODULE user32 = GetModuleHandleA("user32.dll");
    dpi_for_window_fn dpi_for_window =
        (user32 != NULL) ? (dpi_for_window_fn)(void *)GetProcAddress(
                               user32, "GetDpiForWindow")
                         : NULL;
    metrics_for_dpi_fn metrics =
        (user32 != NULL) ? (metrics_for_dpi_fn)(void *)GetProcAddress(
                               user32, "GetSystemMetricsForDpi")
                         : NULL;
    NCCALCSIZE_PARAMS *params = (NCCALCSIZE_PARAMS *)lParam;
    int fx;
    int fy;
    if ((dpi_for_window != NULL) && (metrics != NULL)) {
      UINT dpi = dpi_for_window(hwnd);
      fx = metrics(SM_CXFRAME, dpi) + metrics(92 /* SM_CXPADDEDBORDER */, dpi);
      fy = metrics(SM_CYFRAME, dpi) + metrics(92 /* SM_CXPADDEDBORDER */, dpi);
    } else {
      fx = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(92);
      fy = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(92);
    }
    params->rgrc[0].left += fx;
    params->rgrc[0].right -= fx;
    params->rgrc[0].top += fy;
    params->rgrc[0].bottom -= fy;
  }
  *result = 0;
  return 1;
}
