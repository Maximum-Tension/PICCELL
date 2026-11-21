/* ---------------------------------------------------------------------------
 * wv_backdrop.inl - glass the page draws itself
 *
 * Windows' own materials (wv_glass.inl) blur with a fixed strength, cannot
 * bend what is behind them, and can be switched off by the user. This is the
 * other way round: C takes a live picture of the desktop behind the window
 * and hands it to the page, and the page does whatever it likes with it -
 * CSS blur at any strength, SVG displacement, lenses, per-element glass.
 *
 *     webview_backdrop_start(&w, 30, 4, 48);   30 fps, quarter size, 48px
 *                                               of desktop around the window
 *     webview_backdrop_stop(&w);
 *
 * Use it with a transparent window (WEBVIEW_WINDOW_ALPHA), so the page can
 * draw its glass with smooth rounded corners and a CSS shadow and leave the
 * rest of the window clear. examples/10_glass starts that way ("Drawn by
 * this page").
 *
 * HOW: the picture has to show what is BEHIND the window, not the window.
 * Two ways to get that, tried in this order (webview_backdrop_set_source):
 *
 *  1. The magnifier (the default). Windows' Magnification API renders a
 *     piece of the desktop the way the system Magnifier does, and takes a
 *     list of windows to leave out - of that one picture only. Screenshots,
 *     recordings, streams and screen sharing all still see the window,
 *     live. See "the magnifier" below.
 *  2. Exclusion, if the magnifier is not available or fails its checks:
 *     the window is left out of ALL screen capture (SetWindowDisplayAffinity,
 *     WDA_EXCLUDEFROMCAPTURE, Windows 10 2004+), and a plain copy of the
 *     screen at its position shows what is behind it. The price: it is gone
 *     from screenshots and recordings too - see "who can see it".
 *
 * The copy is shrunk by `scale` (the page blurs it anyway, and it is far
 * less data) and handed to the page, one of two ways:
 *
 *  - as raw bytes in shared memory, once the page has said it takes them by
 *    posting the message "__wvbd shared" (WebView2 runtime 1.0.1661 or
 *    later). Each frame arrives as window.chrome.webview's
 *    "sharedbufferreceived" event: e.getBuffer() is an ArrayBuffer of RGB
 *    pixels, e.additionalData is {wvbd: 1, w, h, m, seq}. Nothing is
 *    encoded or copied on the way. Call chrome.webview.releaseBuffer() on it
 *    when done.
 *  - otherwise as a string message:
 *
 *     "wvbd1 <width> <height> <margin> <seq>\n" + base64 of RGB pixels
 *
 * Either way it is width x height pixels covering the client area plus
 * `margin` CSS pixels on every side, row by row, 3 bytes per pixel (in the
 * text, exactly 4 base64 characters per pixel). "wvbd0" means the backdrop
 * stopped. examples/10_glass has a reader for both.
 *
 * THE COST, which is real:
 *  - With exclusion (only), the window is hidden from screenshots and screen
 *    sharing. Who sees it is a policy - see "who can see it" below.
 *  - Every frame is a copy of a piece of the screen. Unchanged frames are not
 *    sent, and after a second of nothing changing it looks a third as often,
 *    so a still desktop costs little; while the window moves or something
 *    plays behind it, it is a few milliseconds per frame.
 *  - Captures are paced by the display: a small thread waits for each
 *    refresh of the monitor the window is on and asks for a capture then -
 *    120 a second on a 120 Hz screen, at most the rate asked for, fewer
 *    when a frame takes too long (see "the pace"). A timer does it where
 *    that wait is not available.
 *  - The picture is one frame behind while the window is dragged.
 *  - webview_backdrop_status() says which way it went, the display's rate,
 *    how many frames a second it really captures and how long one takes.
 *
 * Included by webview.h after wv_options.inl. Not meant to be compiled on its
 * own.
 * -------------------------------------------------------------------------*/

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif
#define WEBVIEW_WDA_EXCLUDEFROMCAPTURE 0x00000011

static struct {
  int on;
  int fps;          /* what webview_backdrop_start() asked for          */
  int fps_focused;  /* webview_backdrop_set_fps(): 0 = use fps          */
  int fps_background;
  int fps_now;      /* the rate the timer runs at right now            */
  int scale;       /* 1 = full size, 4 = a quarter in each direction */
  int margin;      /* CSS pixels of desktop around the client area      */
  unsigned seq;
  DWORD last_tick; /* throttles captures triggered by moves             */
  unsigned char *full; /* the screen copy, BGRA, reused                 */
  size_t full_size;
  unsigned char *rgb;  /* the shrunk frame, RGB                         */
  unsigned char *last; /* the previous frame sent, to skip repeats      */
  size_t rgb_size;
  int last_w, last_h;
  int unchanged;   /* frames in a row with nothing new behind the window */
  unsigned ticks;
  int source;      /* WEBVIEW_BACKDROP_SOURCE_*: what was asked for       */
  int route;       /* what runs: _MAGNIFIER, _EXCLUSION, or 0 when off    */
  char route_why[96]; /* why the magnifier is not the one running         */
  int verified;    /* the magnifier was checked against the real screen   */
  int verify_tries; /* ...and looked wrong this many times in a row       */
  DWORD verify_at;
  int mag_failures; /* frames in a row the magnifier returned nothing     */
  unsigned sent;   /* frames sent since starting                         */
  double copy_ms;  /* time a capture takes, all of it, smoothed           */
  double carry;    /* the pace: refreshes owed a capture, see there      */
  unsigned due;    /* captures that came due                             */
  int moved;       /* the window moved since the last capture            */
  unsigned captured;   /* captures in the second being counted...        */
  DWORD captured_since;
  int fps_real;    /* ...and in the last whole second                    */
  int shared;      /* frames as shared memory: 0 the page has not asked,
                      1 yes, 2 asked but this runtime cannot */
  int policy;      /* WEBVIEW_BACKDROP_SCREENSHOTS, _HIDDEN or _VISIBLE   */
  int excluded;    /* left out of capture right now                     */
  int revealed;    /* a screenshot key was pressed: visible, held still */
  int sizemove;    /* the user is dragging or resizing the window       */
  struct webview *w; /* for the keyboard hook                           */
  HHOOK hook;
  WCHAR *message;
  size_t message_size;
  char status[160];
} webview_backdrop;

static int webview_pacer_running(void);
static int webview_pacer_hz(void);
static int webview_pacer_how(void);

/* What the backdrop is doing, in a line - "off", "running ...", or why it
   could not start. */
WEBVIEW_API const char *webview_backdrop_status(void) {
  static char line[400];
  int focused, behind;
  if (!webview_backdrop.on) {
    return webview_backdrop.status[0] ? webview_backdrop.status : "off";
  }
  focused = webview_backdrop.fps_focused ? webview_backdrop.fps_focused
                                         : webview_backdrop.fps;
  behind = webview_backdrop.fps_background ? webview_backdrop.fps_background
                                           : webview_backdrop.fps;
  snprintf(line, sizeof(line),
           "running: %s - %s. %s %d Hz; asked for %d fps focused, %d behind; "
           "capturing %d a second, %.1f ms each; 1/%d size, %d px margin; "
           "sent as %s%s%s",
           (webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER)
               ? "magnifier"
               : "exclusion",
           (webview_backdrop.policy == WEBVIEW_BACKDROP_HIDDEN)
               ? "hidden from all capture, by choice"
           : (webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER)
               ? "seen by screenshots, recordings and screen sharing"
           : (webview_backdrop.policy == WEBVIEW_BACKDROP_VISIBLE)
               ? "visible to capture, picture refreshed every 2 s"
               : "hidden from capture, shows up when a screenshot key is "
                 "pressed",
           !webview_pacer_running()   ? "timer; display"
           : (webview_pacer_how() == 2) ? "paced by a timer at the display's rate,"
                                        : "paced by the display's refresh,",
           webview_pacer_hz(), focused, behind, webview_backdrop.fps_real,
           webview_backdrop.copy_ms, webview_backdrop.scale,
           webview_backdrop.margin,
           (webview_backdrop.shared == 1) ? "shared memory" : "text",
           webview_backdrop.route_why[0] ? ". Not the magnifier: " : "",
           webview_backdrop.route_why);
  return line;
}

/* Which way the picture is taken right now: WEBVIEW_BACKDROP_SOURCE_MAGNIFIER,
   _EXCLUSION, or 0 when the backdrop is off. */
WEBVIEW_API int webview_backdrop_source(void) {
  return webview_backdrop.on ? webview_backdrop.route : 0;
}

static void webview_backdrop_free(void) {
  free(webview_backdrop.full);
  free(webview_backdrop.rgb);
  free(webview_backdrop.last);
  free(webview_backdrop.message);
  webview_backdrop.full = NULL;
  webview_backdrop.rgb = NULL;
  webview_backdrop.last = NULL;
  webview_backdrop.message = NULL;
  webview_backdrop.full_size = 0;
  webview_backdrop.rgb_size = 0;
  webview_backdrop.message_size = 0;
  webview_backdrop.last_w = 0;
  webview_backdrop.last_h = 0;
}

static int webview_backdrop_post(struct webview *w, const WCHAR *message) {
  ICoreWebView2 *view;
  if (!webview_webview2_enabled || (w->priv.webview2 == NULL) ||
      (w->priv.webview2->webview == NULL)) {
    return -1;
  }
  view = w->priv.webview2->webview;
  return SUCCEEDED(view->lpVtbl->PostWebMessageAsString(view, message)) ? 0
                                                                         : -1;
}

/* --------------------------------------------- the fast way to the page
 *
 * As text, a frame is 4 base64 characters per pixel in UTF-16 - 8 bytes for
 * every 3 - encoded here, copied across, and decoded again in the page. A
 * WebView2 shared buffer is memory both processes map: the bytes are
 * written once and the page reads them in place as an ArrayBuffer. The
 * bundled WebView2.h predates the API, so its three interfaces are spelled
 * out here; slot order as in the SDK's WebView2.idl (ICoreWebView2_15 adds
 * four methods to _14, _16 three, _17 PostSharedBufferToScript;
 * Environment11 adds one to Environment10, Environment12 CreateSharedBuffer).
 * -------------------------------------------------------------------------*/
static const IID webview_iid_webview2_17 = {
    0x702e75d4, 0xfd44, 0x434d, {0x9d, 0x70, 0x1a, 0x68, 0xa6, 0xb1, 0x19, 0x2a}};
static const IID webview_iid_environment12 = {
    0xf503db9b, 0x739f, 0x48dd, {0xb1, 0x51, 0xfd, 0xfc, 0xf2, 0x53, 0xf5, 0x4e}};

typedef struct webview_shared_buffer webview_shared_buffer;
typedef struct {
  HRESULT(STDMETHODCALLTYPE *QueryInterface)(webview_shared_buffer *, REFIID,
                                             void **);
  ULONG(STDMETHODCALLTYPE *AddRef)(webview_shared_buffer *);
  ULONG(STDMETHODCALLTYPE *Release)(webview_shared_buffer *);
  HRESULT(STDMETHODCALLTYPE *get_Size)(webview_shared_buffer *, UINT64 *);
  HRESULT(STDMETHODCALLTYPE *get_Buffer)(webview_shared_buffer *, BYTE **);
  HRESULT(STDMETHODCALLTYPE *OpenStream)(webview_shared_buffer *, IStream **);
  HRESULT(STDMETHODCALLTYPE *get_FileMappingHandle)(webview_shared_buffer *,
                                                    HANDLE *);
  HRESULT(STDMETHODCALLTYPE *Close)(webview_shared_buffer *);
} webview_shared_buffer_vtbl;
struct webview_shared_buffer {
  const webview_shared_buffer_vtbl *lpVtbl;
};

typedef struct webview_environment12 webview_environment12;
typedef struct {
  ICoreWebView2Environment10Vtbl base;
  void *get_FailureReportFolderPath; /* Environment11 */
  HRESULT(STDMETHODCALLTYPE *CreateSharedBuffer)(webview_environment12 *,
                                                 UINT64,
                                                 webview_shared_buffer **);
} webview_environment12_vtbl;
struct webview_environment12 {
  const webview_environment12_vtbl *lpVtbl;
};

typedef struct webview_webview2_17 webview_webview2_17;
typedef struct {
  ICoreWebView2_14Vtbl base;
  void *add_FaviconChanged; /* _15 */
  void *remove_FaviconChanged;
  void *get_FaviconUri;
  void *GetFavicon;
  void *Print; /* _16 */
  void *ShowPrintUI;
  void *PrintToPdfStream;
  HRESULT(STDMETHODCALLTYPE *PostSharedBufferToScript)(webview_webview2_17 *,
                                                       webview_shared_buffer *,
                                                       int /* access */,
                                                       LPCWSTR);
} webview_webview2_17_vtbl;
struct webview_webview2_17 {
  const webview_webview2_17_vtbl *lpVtbl;
};

/* The page posted "__wvbd shared": it reads frames from shared buffers. */
static void webview_backdrop_page_takes_shared(void) {
  if (webview_backdrop.shared == 0) {
    webview_backdrop.shared = 1;
  }
}

/* One frame as a shared buffer. Returns -1 if that is not possible - then
   never tried again, and frames go as text. */
static int webview_backdrop_post_shared(struct webview *w,
                                        const unsigned char *rgb, size_t n,
                                        int dw, int dh) {
  webview2 *p = w->priv.webview2;
  webview_environment12 *env = NULL;
  webview_webview2_17 *view = NULL;
  webview_shared_buffer *buffer = NULL;
  BYTE *memory = NULL;
  WCHAR json[128];
  int ok = 0;
  if ((webview_backdrop.shared != 1) || !webview_webview2_enabled ||
      (p == NULL) || (p->env == NULL) || (p->webview == NULL)) {
    return -1;
  }
  webview_step("shared: QueryInterface, CreateSharedBuffer");
  if (SUCCEEDED(p->env->lpVtbl->QueryInterface(
          p->env, iid_unref(&webview_iid_environment12), (void **)&env)) &&
      (env != NULL) &&
      SUCCEEDED(p->webview->lpVtbl->QueryInterface(
          p->webview, iid_unref(&webview_iid_webview2_17), (void **)&view)) &&
      (view != NULL) &&
      SUCCEEDED(env->lpVtbl->CreateSharedBuffer(env, (UINT64)n, &buffer)) &&
      (buffer != NULL) &&
      SUCCEEDED(buffer->lpVtbl->get_Buffer(buffer, &memory)) &&
      (memory != NULL)) {
    webview_step("shared: copying the frame in");
    memcpy(memory, rgb, n);
    _snwprintf(json, sizeof(json) / sizeof(json[0]),
               L"{\"wvbd\":1,\"w\":%d,\"h\":%d,\"m\":%d,\"seq\":%u}", dw, dh,
               webview_backdrop.margin, webview_backdrop.seq);
    json[sizeof(json) / sizeof(json[0]) - 1] = 0;
    webview_step("shared: PostSharedBufferToScript");
    ok = SUCCEEDED(view->lpVtbl->PostSharedBufferToScript(
        view, buffer, 0 /* COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_ONLY */,
        json));
  }
  webview_step("shared: releasing");
  /* the page holds its own reference until it calls releaseBuffer() */
  if (buffer != NULL) {
    buffer->lpVtbl->Release(buffer);
  }
  if (view != NULL) {
    view->lpVtbl->base.Release((ICoreWebView2_14 *)(void *)view);
  }
  if (env != NULL) {
    env->lpVtbl->base.Release((ICoreWebView2Environment10 *)(void *)env);
  }
  if (!ok) {
    webview_backdrop.shared = 2;
    webview_print_log("backdrop: shared buffers not available (WebView2 "
                      "runtime before 1.0.1661?); frames go as text");
    return -1;
  }
  return 0;
}

/* Grow a buffer to at least `need` bytes. */
static int webview_backdrop_reserve(void **buffer, size_t *size, size_t need) {
  void *p;
  if (*size >= need) {
    return 0;
  }
  p = realloc(*buffer, need);
  if (p == NULL) {
    return -1;
  }
  *buffer = p;
  *size = need;
  return 0;
}

/* ------------------------------------------------------- the magnifier
 *
 * magnification.dll renders a rectangle of the desktop for the magnifier
 * control, and MagSetWindowFilterList takes windows to leave out of what it
 * renders. Only out of that: every other capture still sees them. So the
 * magnifier is asked for the rectangle behind the window, minus the window,
 * and the pixels come back through MagSetImageScalingCallback - a call
 * Microsoft marks deprecated but still ships and runs; WebRTC (Chrome's
 * screen capture) uses this same path to leave windows out.
 *
 * Besides the window itself, every window IN FRONT of it that crosses the
 * captured area is left out too (webview_mag_front): the glass shows what
 * is behind it, not a blurred copy of the window lying on top of it.
 *
 * Its quirks, handled here:
 *  - it renders one monitor at a time reliably: a rectangle that crosses
 *    monitors is asked for in one piece per monitor;
 *  - a magnifier window is never resized once it has a source: resizing
 *    makes it render again with the old source into the new size, and
 *    magnification.dll reads past its own buffer and takes the process
 *    down (seen on Windows 11, Magnification.dll+0xe28c). When the size of
 *    a piece changes - the window was resized, or it crossed onto another
 *    monitor - that piece gets a new magnifier window of the right size
 *    instead. One window per piece, so two monitors do not take turns.
 *    Sizes only grow, in steps of 64 pixels: the source rectangle is made
 *    that big, kept inside its monitor, and only the piece is copied out.
 *    So a window sliding into a screen edge - where its piece keeps
 *    shrinking - keeps its magnifier, and dragging the window's edge makes
 *    a new one every 64 pixels at most;
 *  - the magnifier windows sit at the screen's origin, only the source
 *    rectangle moves: moved around, across the edge of the desktop, it has
 *    been seen to return nothing;
 *  - the callback has no user pointer: it writes to webview_mag.dst, set
 *    just before the (synchronous) MagSetWindowSource call;
 *  - the host window must exist but never be shown;
 *  - if it ever hands back nothing, all black, or the window itself, the
 *    exclusion route takes over (webview_backdrop_fall_back).
 * -------------------------------------------------------------------------*/

typedef struct { /* MAGIMAGEHEADER, without needing magnification.h */
  UINT width;
  UINT height;
  GUID format;
  UINT stride;
  UINT offset;
  SIZE_T cbSize;
} webview_mag_header;
typedef BOOL(CALLBACK *webview_mag_image_fn)(HWND, void *, webview_mag_header,
                                             void *, webview_mag_header, RECT,
                                             RECT, HRGN);
typedef BOOL(WINAPI *webview_mag_void_fn)(void);
typedef BOOL(WINAPI *webview_mag_source_fn)(HWND, RECT);
typedef BOOL(WINAPI *webview_mag_filter_fn)(HWND, DWORD, int, HWND *);
typedef BOOL(WINAPI *webview_mag_callback_fn)(HWND, webview_mag_image_fn);

static struct {
  HMODULE dll;
  webview_mag_void_fn init, uninit;
  webview_mag_source_fn source;
  webview_mag_filter_fn filter;
  webview_mag_callback_fn callback;
  int initialized;
  HWND host;
  HWND view[8];        /* one magnifier window per piece...            */
  int view_w[8], view_h[8]; /* ...and the size it was made with, for good */
  HWND excluded[32];   /* left out: the window, then those in front of it */
  int excluded_count;
  /* where the callback puts the pixels of the piece being rendered, and
     where in the rendered picture the piece starts */
  unsigned char *dst;
  int dst_stride, want_w, want_h, from_x, from_y, got;
} webview_mag;

static BOOL CALLBACK webview_mag_on_image(HWND hwnd, void *src,
                                          webview_mag_header head, void *dest,
                                          webview_mag_header dest_head,
                                          RECT unclipped, RECT clipped,
                                          HRGN dirty) {
  int y;
  size_t row;
  (void)hwnd;
  (void)dest;
  (void)dest_head;
  (void)unclipped;
  (void)clipped;
  (void)dirty;
  if ((webview_mag.dst == NULL) || (src == NULL) ||
      ((int)head.width < webview_mag.from_x + webview_mag.want_w) ||
      ((int)head.height < webview_mag.from_y + webview_mag.want_h) ||
      (head.stride < head.width * 4) ||
      (head.cbSize < (SIZE_T)head.stride * (head.height - 1) + head.width * 4)) {
    return TRUE; /* not what was asked for: leave got at 0 */
  }
  webview_step("magnifier: copying the callback's pixels");
  /* 32-bit pixels, B G R A in memory like a GDI DIB, top row first */
  row = (size_t)webview_mag.want_w * 4;
  for (y = 0; y < webview_mag.want_h; y++) {
    memcpy(webview_mag.dst + (size_t)y * webview_mag.dst_stride,
           (const unsigned char *)src +
               (size_t)(webview_mag.from_y + y) * head.stride +
               (size_t)webview_mag.from_x * 4,
           row);
  }
  webview_mag.got = 1;
  return TRUE;
}

static void webview_mag_drop_view(int i) {
  if (webview_mag.view[i] != NULL) {
    DestroyWindow(webview_mag.view[i]);
  }
  webview_mag.view[i] = NULL;
  webview_mag.view_w[i] = 0;
  webview_mag.view_h[i] = 0;
}

/* A magnifier window of exactly w x h for piece i, leaving the excluded
   window out: the one there if it is that size, else a new one. Never a
   resize - see the quirks above. */
static HWND webview_mag_view(int i, int w, int h) {
  HWND v;
  if ((webview_mag.view[i] != NULL) && (webview_mag.view_w[i] == w) &&
      (webview_mag.view_h[i] == h)) {
    return webview_mag.view[i];
  }
  webview_step("magnifier: a window for a new size");
  webview_mag_drop_view(i);
  v = CreateWindowExW(0, L"Magnifier", L"", WS_CHILD | WS_VISIBLE, 0, 0, w,
                      h, webview_mag.host, NULL, GetModuleHandleW(NULL), NULL);
  if (v == NULL) {
    return NULL;
  }
  if (!webview_mag.callback(v, webview_mag_on_image) ||
      !webview_mag.filter(v, 0 /* MW_FILTERMODE_EXCLUDE */,
                          webview_mag.excluded_count, webview_mag.excluded)) {
    DestroyWindow(v);
    return NULL;
  }
  webview_mag.view[i] = v;
  webview_mag.view_w[i] = w;
  webview_mag.view_h[i] = h;
  return v;
}

static void webview_mag_close(void) {
  int i;
  for (i = 0; i < 8; i++) {
    webview_mag_drop_view(i); /* before MagUninitialize */
  }
  if (webview_mag.host != NULL) {
    DestroyWindow(webview_mag.host);
  }
  webview_mag.host = NULL;
  webview_mag.excluded_count = 0;
  if (webview_mag.initialized && (webview_mag.uninit != NULL)) {
    webview_mag.uninit();
  }
  webview_mag.initialized = 0;
}

/* Load the API and make the (never shown) magnifier, leaving `exclude` out
   of what it renders. Returns 0, or -1 with the reason in `why`. */
static int webview_mag_open(HWND exclude, char *why, size_t why_size) {
  HINSTANCE instance = GetModuleHandleW(NULL);
  WNDCLASSEXW wc;
  HWND test;
  int i;
  if (webview_mag.host != NULL) {
    if ((webview_mag.excluded_count < 1) || (webview_mag.excluded[0] != exclude)) {
      /* the windows carry the filter: new ones for the new window */
      for (i = 0; i < 8; i++) {
        webview_mag_drop_view(i);
      }
      webview_mag.excluded[0] = exclude;
      webview_mag.excluded_count = 1;
    }
    return 0;
  }
  webview_step("magnifier: loading Magnification.dll");
  if (webview_mag.dll == NULL) {
    webview_mag.dll = LoadLibraryW(L"Magnification.dll");
  }
  if (webview_mag.dll == NULL) {
    snprintf(why, why_size, "no Magnification.dll");
    return -1;
  }
  webview_mag.init = (webview_mag_void_fn)(void *)GetProcAddress(
      webview_mag.dll, "MagInitialize");
  webview_mag.uninit = (webview_mag_void_fn)(void *)GetProcAddress(
      webview_mag.dll, "MagUninitialize");
  webview_mag.source = (webview_mag_source_fn)(void *)GetProcAddress(
      webview_mag.dll, "MagSetWindowSource");
  webview_mag.filter = (webview_mag_filter_fn)(void *)GetProcAddress(
      webview_mag.dll, "MagSetWindowFilterList");
  webview_mag.callback = (webview_mag_callback_fn)(void *)GetProcAddress(
      webview_mag.dll, "MagSetImageScalingCallback");
  if ((webview_mag.init == NULL) || (webview_mag.uninit == NULL) ||
      (webview_mag.source == NULL) || (webview_mag.filter == NULL) ||
      (webview_mag.callback == NULL)) {
    snprintf(why, why_size, "Magnification.dll lacks a function");
    return -1;
  }
  webview_step("magnifier: MagInitialize");
  if (!webview_mag.init()) {
    snprintf(why, why_size, "MagInitialize failed (error %lu)",
             (unsigned long)GetLastError());
    return -1;
  }
  webview_mag.initialized = 1;
  ZeroMemory(&wc, sizeof(wc));
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = instance;
  wc.lpszClassName = L"webview_magnifier_host";
  webview_step("magnifier: creating its windows");
  RegisterClassExW(&wc); /* fails harmlessly the second time */
  webview_mag.host = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                                     L"webview_magnifier_host", L"", WS_POPUP,
                                     0, 0, 0, 0, NULL, NULL, instance, NULL);
  if (webview_mag.host == NULL) {
    snprintf(why, why_size, "could not make the magnifier (error %lu)",
             (unsigned long)GetLastError());
    webview_mag_close();
    return -1;
  }
  /* one window now, to know the callback and the filter are taken; it is
     thrown away unused (it never gets a source) */
  webview_step("magnifier: setting the callback and the filter");
  webview_mag.excluded[0] = exclude;
  webview_mag.excluded_count = 1;
  test = webview_mag_view(0, 1, 1);
  if (test == NULL) {
    snprintf(why, why_size, "the magnifier refused its setup (error %lu)",
             (unsigned long)GetLastError());
    webview_mag_close();
    return -1;
  }
  webview_mag_drop_view(0);
  return 0;
}

/* piece i, inside monitor `mon`, into bits at (ox, oy) */
static int webview_mag_piece(int i, unsigned char *bits, int stride,
                             RECT piece, RECT mon, int ox, int oy) {
  int ok;
  int w = piece.right - piece.left, h = piece.bottom - piece.top;
  int mw = mon.right - mon.left, mh = mon.bottom - mon.top;
  /* the rendered size: up to the next 64, no smaller than this piece's
     magnifier already is (sizes only grow), no bigger than the monitor */
  int rw = (w + 63) & ~63, rh = (h + 63) & ~63;
  RECT source;
  HWND view;
  if ((webview_mag.view[i] != NULL) && (webview_mag.view_w[i] <= mw) &&
      (webview_mag.view_h[i] <= mh)) {
    if (webview_mag.view_w[i] > rw) rw = webview_mag.view_w[i];
    if (webview_mag.view_h[i] > rh) rh = webview_mag.view_h[i];
  }
  if (rw > mw) rw = mw;
  if (rh > mh) rh = mh;
  if ((rw < w) || (rh < h)) {
    return -1; /* cannot happen: the piece is inside the monitor */
  }
  /* ...placed over the piece, pushed back inside the monitor */
  source.left = (piece.left + rw > mon.right) ? mon.right - rw : piece.left;
  source.top = (piece.top + rh > mon.bottom) ? mon.bottom - rh : piece.top;
  source.right = source.left + rw;
  source.bottom = source.top + rh;
  view = webview_mag_view(i, rw, rh);
  if (view == NULL) {
    return -1;
  }
  webview_mag.dst = bits + (size_t)oy * stride + (size_t)ox * 4;
  webview_mag.dst_stride = stride;
  webview_mag.want_w = w;
  webview_mag.want_h = h;
  webview_mag.from_x = piece.left - source.left;
  webview_mag.from_y = piece.top - source.top;
  webview_mag.got = 0;
  webview_step("magnifier: MagSetWindowSource");
  ok = webview_mag.source(view, source) && webview_mag.got;
  webview_step("magnifier: piece done");
  webview_mag.dst = NULL;
  return ok ? 0 : -1;
}

struct webview_mag_pieces {
  RECT want;
  RECT piece[8];
  RECT monitor[8];
  int count;
  long long area;
};

static BOOL CALLBACK webview_mag_monitor(HMONITOR monitor, HDC dc, LPRECT r,
                                         LPARAM data) {
  struct webview_mag_pieces *p = (struct webview_mag_pieces *)data;
  RECT part;
  MONITORINFO mi;
  (void)dc;
  /* the whole monitor, not `r`: some systems pass it already clipped */
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(monitor, &mi)) {
    mi.rcMonitor = *r;
  }
  if ((p->count < 8) && IntersectRect(&part, &mi.rcMonitor, &p->want)) {
    p->monitor[p->count] = mi.rcMonitor;
    p->piece[p->count++] = part;
    p->area += (long long)(part.right - part.left) * (part.bottom - part.top);
  }
  return TRUE;
}

/* What to leave out: the window, and every visible window in front of it
   that crosses `area`. The magnifier windows are told when that changes. */
static void webview_mag_front(RECT area) {
  HWND list[32];
  HWND self = webview_mag.excluded[0];
  HWND h;
  int n = 1, i;
  if ((self == NULL) || (webview_mag.excluded_count < 1)) {
    return;
  }
  list[0] = self;
  for (h = GetWindow(self, GW_HWNDPREV); (h != NULL) && (n < 32);
       h = GetWindow(h, GW_HWNDPREV)) {
    RECT r, x;
    if ((h == webview_mag.host) || !IsWindowVisible(h) || IsIconic(h) ||
        !GetWindowRect(h, &r) || !IntersectRect(&x, &r, &area)) {
      continue;
    }
    list[n++] = h;
  }
  if ((n == webview_mag.excluded_count) &&
      (memcmp(list, webview_mag.excluded, (size_t)n * sizeof(HWND)) == 0)) {
    return;
  }
  webview_step("magnifier: new windows in front");
  memcpy(webview_mag.excluded, list, (size_t)n * sizeof(HWND));
  webview_mag.excluded_count = n;
  for (i = 0; i < 8; i++) {
    if ((webview_mag.view[i] != NULL) &&
        !webview_mag.filter(webview_mag.view[i], 0 /* EXCLUDE */, n, list)) {
      webview_mag_drop_view(i); /* made again, with the list, when needed */
    }
  }
}

/* The desktop at (sx, sy, sw, sh) without the excluded windows, BGRA, into
   bits. Off-screen parts are black, as a screen copy would have them. */
static int webview_mag_copy(unsigned char *bits, int sx, int sy, int sw,
                            int sh) {
  struct webview_mag_pieces p;
  int i;
  ZeroMemory(&p, sizeof(p));
  SetRect(&p.want, sx, sy, sx + sw, sy + sh);
  webview_mag_front(p.want);
  webview_step("magnifier: finding the monitors");
  EnumDisplayMonitors(NULL, &p.want, webview_mag_monitor, (LPARAM)&p);
  if (p.area < (long long)sw * sh) { /* off screen, all or in part */
    memset(bits, 0, (size_t)sw * sh * 4);
  }
  for (i = 0; i < p.count; i++) {
    if (webview_mag_piece(i, bits, sw * 4, p.piece[i], p.monitor[i],
                          p.piece[i].left - sx, p.piece[i].top - sy) != 0) {
      return -1;
    }
  }
  return 0;
}

/* ------------------------------------------------------- the screen copy */

static void webview_backdrop_fall_back(struct webview *w, const char *why);

/* A plain copy of the screen: with exclusion, the window is not in it. */
static int webview_backdrop_bitblt(unsigned char *bits, int sx, int sy,
                                   int sw, int sh) {
  HDC screen, mem;
  HBITMAP dib, old;
  BITMAPINFO bi;
  void *pixels = NULL;
  int ok = 0;
  ZeroMemory(&bi, sizeof(bi));
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = sw;
  bi.bmiHeader.biHeight = -sh; /* top-down */
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  webview_step("screen copy: BitBlt");
  screen = GetDC(NULL);
  if (screen == NULL) {
    return -1;
  }
  mem = CreateCompatibleDC(screen);
  dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &pixels, NULL, 0);
  if ((mem != NULL) && (dib != NULL) && (pixels != NULL)) {
    old = (HBITMAP)SelectObject(mem, dib);
    /* No CAPTUREBLT: it makes the cursor flicker, and under DWM the screen
       DC already holds the composed desktop. */
    ok = BitBlt(mem, 0, 0, sw, sh, screen, sx, sy, SRCCOPY);
    GdiFlush();
    if (ok) {
      memcpy(bits, pixels, (size_t)sw * sh * 4);
    }
    SelectObject(mem, old);
  }
  if (dib != NULL) {
    DeleteObject(dib);
  }
  if (mem != NULL) {
    DeleteDC(mem);
  }
  ReleaseDC(NULL, screen);
  return ok ? 0 : -1;
}

/* Something about the magnifier's picture looked wrong: look again in two
   seconds - a window that has not painted yet looks like the desktop - and
   give up on it the third time in a row. */
static void webview_mag_suspect(struct webview *w, const char *why) {
  if (++webview_backdrop.verify_tries >= 3) {
    webview_backdrop_fall_back(w, why);
  } else {
    webview_backdrop.verify_at = GetTickCount() + 2000;
  }
}

/* Is the magnifier really leaving the window out? Checked once, with the
   window on screen and the page busy for a while: a plain screen copy of
   the client area (which shows the window) must differ from the
   magnifier's (which must not). Identical means the window is in the
   magnifier's picture - the glass would show itself, mirror in mirror. */
static void webview_mag_verify(struct webview *w, const unsigned char *bits,
                               int sx, int sy, int sw, int sh) {
  RECT client;
  POINT origin = {0, 0};
  unsigned char *plain;
  int cx, cy, cw, ch, y, same = 1, mag_black = 1, plain_black = 1;
  size_t i, n = (size_t)sw * sh * 4;
  webview_step("magnifier: checking it against a plain copy");
  if (webview_backdrop.excluded) {
    webview_backdrop.verified = 1; /* hidden by choice: a plain copy would
                                      not show the window either */
    return;
  }
  plain = (unsigned char *)malloc(n);
  if (plain == NULL) {
    return;
  }
  if (webview_backdrop_bitblt(plain, sx, sy, sw, sh) != 0) {
    free(plain);
    return;
  }
  /* all black where the screen is not: the magnifier draws nothing here.
     (A black desktop behind a window that shows nothing is fine.) */
  for (i = 0; (i < n) && (mag_black || plain_black); i += 4) {
    if (bits[i] | bits[i + 1] | bits[i + 2]) {
      mag_black = 0;
    }
    if (plain[i] | plain[i + 1] | plain[i + 2]) {
      plain_black = 0;
    }
  }
  if (mag_black && !plain_black) {
    free(plain);
    webview_mag_suspect(w, "it only returned black");
    return;
  }
  /* the client area: the plain copy shows the window, the magnifier must
     not. Exactly the same means the window is in the magnifier's picture. */
  GetClientRect(w->priv.hwnd, &client);
  ClientToScreen(w->priv.hwnd, &origin);
  cx = origin.x - sx;
  cy = origin.y - sy;
  cw = client.right;
  ch = client.bottom;
  if ((cx >= 0) && (cy >= 0) && (cx + cw <= sw) && (cy + ch <= sh) &&
      (cw > 0) && (ch > 0) && !mag_black) {
    for (y = 0; (y < ch) && same; y++) {
      size_t at = ((size_t)(cy + y) * sw + cx) * 4;
      same = memcmp(plain + at, bits + at, (size_t)cw * 4) == 0;
    }
    if (same) {
      free(plain);
      webview_mag_suspect(w, "it did not leave the window out");
      return;
    }
  }
  free(plain);
  webview_backdrop.verified = 1;
}

/* Copy the screen behind the window, shrink it, and send it if it changed.
   `force` sends even an unchanged frame (after a move the page needs one).
   With exclusion, the window must be left out of capture while this runs. */
static void webview_backdrop_grab(struct webview *w, int force) {
  static const char b64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  RECT client;
  POINT origin = {0, 0};
  UINT dpi;
  int m, sx, sy, sw, sh, dw, dh, x, y, s;
  unsigned char *bits = NULL;
  size_t rgb_need, header_len, chars;
  char header[96];
  WCHAR *out;
  int i;

  if (!webview_backdrop.on || (w == NULL) || (w->priv.hwnd == NULL) ||
      IsIconic(w->priv.hwnd) || !IsWindowVisible(w->priv.hwnd)) {
    return;
  }
  GetClientRect(w->priv.hwnd, &client);
  ClientToScreen(w->priv.hwnd, &origin);
  dpi = webview_window_dpi(w->priv.hwnd);
  m = MulDiv(webview_backdrop.margin, (int)dpi, 96);
  s = webview_backdrop.scale;
  sx = origin.x - m;
  sy = origin.y - m;
  sw = client.right + 2 * m;
  sh = client.bottom + 2 * m;
  /* whole output pixels only: the copy is rounded down to a multiple of s */
  dw = sw / s;
  dh = sh / s;
  if ((dw < 1) || (dh < 1)) {
    return;
  }
  sw = dw * s;
  sh = dh * s;

  /* the screen copy, full size, BGRA */
  if (webview_backdrop_reserve((void **)&webview_backdrop.full,
                               &webview_backdrop.full_size,
                               (size_t)sw * sh * 4) != 0) {
    return;
  }
  bits = webview_backdrop.full;
  if (webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER) {
    if (webview_mag_copy(bits, sx, sy, sw, sh) != 0) {
      /* a lock screen or a UAC prompt stops it for a while: only a second
         of nothing in a row counts */
      if (++webview_backdrop.mag_failures >= 60) {
        webview_backdrop_fall_back(w, "it stopped returning pictures");
      }
      return;
    }
    webview_backdrop.mag_failures = 0;
    if (!webview_backdrop.verified && (webview_backdrop.sent >= 10) &&
        ((LONG)(GetTickCount() - webview_backdrop.verify_at) >= 0)) {
      webview_mag_verify(w, bits, sx, sy, sw, sh);
      if (webview_backdrop.route != WEBVIEW_BACKDROP_SOURCE_MAGNIFIER) {
        return; /* fell back: the next frame comes the other way */
      }
    }
  } else if (webview_backdrop_bitblt(bits, sx, sy, sw, sh) != 0) {
    return;
  }

  webview_step("backdrop: shrinking the frame");
  /* shrink by averaging s x s blocks, BGRA -> RGB */
  rgb_need = (size_t)dw * dh * 3;
  if ((webview_backdrop_reserve((void **)&webview_backdrop.rgb,
                                &webview_backdrop.rgb_size, rgb_need) != 0)) {
    return;
  }
  {
    unsigned char *dst = webview_backdrop.rgb;
    int area = s * s;
    for (y = 0; y < dh; y++) {
      for (x = 0; x < dw; x++) {
        unsigned r = 0, g = 0, b = 0;
        int yy, xx;
        for (yy = 0; yy < s; yy++) {
          const unsigned char *p =
              bits + ((size_t)(y * s + yy) * sw + (size_t)x * s) * 4;
          for (xx = 0; xx < s; xx++, p += 4) {
            b += p[0];
            g += p[1];
            r += p[2];
          }
        }
        *dst++ = (unsigned char)(r / area);
        *dst++ = (unsigned char)(g / area);
        *dst++ = (unsigned char)(b / area);
      }
    }
  }

  /* nothing changed behind the window: send nothing */
  if (!force && (webview_backdrop.last != NULL) &&
      (webview_backdrop.last_w == dw) && (webview_backdrop.last_h == dh) &&
      (memcmp(webview_backdrop.last, webview_backdrop.rgb, rgb_need) == 0)) {
    webview_backdrop.unchanged++;
    return;
  }
  webview_backdrop.unchanged = 0;
  {
    unsigned char *copy = (unsigned char *)realloc(webview_backdrop.last,
                                                   rgb_need);
    if (copy != NULL) {
      memcpy(copy, webview_backdrop.rgb, rgb_need);
      webview_backdrop.last = copy;
      webview_backdrop.last_w = dw;
      webview_backdrop.last_h = dh;
    }
  }

  ++webview_backdrop.seq;
  webview_backdrop.sent++;
  if (webview_backdrop_post_shared(w, webview_backdrop.rgb, rgb_need, dw,
                                   dh) == 0) {
    return;
  }
  webview_step("backdrop: posting the frame as text");
  /* header + base64, straight into the UTF-16 message */
  snprintf(header, sizeof(header), "wvbd1 %d %d %d %u\n", dw, dh,
           webview_backdrop.margin, webview_backdrop.seq);
  header_len = strlen(header);
  chars = header_len + (size_t)dw * dh * 4 + 1;
  if (webview_backdrop_reserve((void **)&webview_backdrop.message,
                               &webview_backdrop.message_size,
                               chars * sizeof(WCHAR)) != 0) {
    return;
  }
  out = webview_backdrop.message;
  for (i = 0; i < (int)header_len; i++) {
    *out++ = (WCHAR)header[i];
  }
  {
    const unsigned char *p = webview_backdrop.rgb;
    const unsigned char *end = p + rgb_need;
    for (; p < end; p += 3) {
      unsigned v = ((unsigned)p[0] << 16) | ((unsigned)p[1] << 8) | p[2];
      *out++ = (WCHAR)b64[(v >> 18) & 63];
      *out++ = (WCHAR)b64[(v >> 12) & 63];
      *out++ = (WCHAR)b64[(v >> 6) & 63];
      *out++ = (WCHAR)b64[v & 63];
    }
  }
  *out = 0;
  webview_backdrop_post(w, webview_backdrop.message);
}

/* ----------------------------------------------------- who can see it
 *
 * With the magnifier there is nothing to trade: the window is in every
 * screenshot, recording and share, and the glass is live. Only HIDDEN means
 * something there - it leaves the window out of all capture, if you want
 * that.
 *
 * With exclusion, leaving the window out of capture is how it can
 * photograph what is behind it - and the same flag hides it from screenshots
 * and screen sharing. So when it is left out is a policy
 * (webview_backdrop_set_capture):
 *
 *   WEBVIEW_BACKDROP_SCREENSHOTS  (the default) left out and live, but a
 *        screenshot key - Print Screen in any combination, Win+Shift+S -
 *        puts it back in capture before the screenshot tool takes its
 *        picture. The glass holds still for a few seconds, then goes live
 *        again. Watching for those keys takes a low-level keyboard hook
 *        (WH_KEYBOARD_LL). It looks at those two combinations and nothing
 *        else, and passes every key on untouched.
 *   WEBVIEW_BACKDROP_VISIBLE  always in capture, for screen sharing and
 *        recording. Left out for one frame at a time, only to refresh the
 *        picture: every 2 seconds, and throughout a drag or resize.
 *        Viewers of a recording may see the window blink at those moments.
 *   WEBVIEW_BACKDROP_HIDDEN  always left out, always live. Invisible to
 *        every screenshot, recording and share.
 * -------------------------------------------------------------------------*/

#define WEBVIEW_BACKDROP_REVEAL_MS 5000 /* how long a screenshot key shows it */
#define WEBVIEW_BACKDROP_VISIBLE_MS 2000 /* VISIBLE: picture refresh interval */

static void webview_backdrop_affinity(struct webview *w, int exclude) {
  typedef BOOL(WINAPI * affinity_fn)(HWND, DWORD);
  HMODULE user32 = GetModuleHandleA("user32.dll");
  affinity_fn set_affinity =
      (user32 != NULL) ? (affinity_fn)(void *)GetProcAddress(
                             user32, "SetWindowDisplayAffinity")
                       : NULL;
  exclude = exclude ? 1 : 0;
  if ((set_affinity == NULL) || (w == NULL) || (w->priv.hwnd == NULL) ||
      (webview_backdrop.excluded == exclude)) {
    return;
  }
  if (set_affinity(w->priv.hwnd,
                   exclude ? WEBVIEW_WDA_EXCLUDEFROMCAPTURE : WDA_NONE)) {
    webview_backdrop.excluded = exclude;
  }
}

/* Wait for DWM to compose a frame with the new affinity, so the next screen
   copy really leaves the window out. */
static void webview_backdrop_dwm_flush(void) {
  typedef HRESULT(WINAPI * flush_fn)(void);
  HMODULE dwm = webview_glass_dwm();
  flush_fn flush = (dwm != NULL) ? (flush_fn)(void *)GetProcAddress(
                                       dwm, "DwmFlush")
                                 : NULL;
  if (flush != NULL) {
    flush();
    flush();
  }
}

/* The policy-aware capture: everything else calls this, not the grab. */
static void webview_backdrop_capture(struct webview *w, int force) {
  int temporary = 0;
  LARGE_INTEGER t0, t1, freq;
  DWORD now;
  if (!webview_backdrop.on || webview_backdrop.revealed) {
    return; /* a screenshot is being taken: the glass holds still */
  }
  QueryPerformanceCounter(&t0);
  webview_backdrop.moved = 0;
  if (webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER) {
    webview_backdrop_grab(w, force); /* the window can stay where it is */
  } else {
    if (!webview_backdrop.excluded) {
      /* VISIBLE: out of capture for this one frame only */
      webview_backdrop_affinity(w, 1);
      webview_backdrop_dwm_flush();
      temporary = 1;
    }
    webview_backdrop_grab(w, force);
    if (temporary) {
      webview_backdrop_affinity(w, 0);
    }
  }
  /* how long that took, all of it: copy, shrink, hand over */
  QueryPerformanceCounter(&t1);
  QueryPerformanceFrequency(&freq);
  if (freq.QuadPart > 0) {
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 /
                (double)freq.QuadPart;
    webview_backdrop.copy_ms = (webview_backdrop.copy_ms <= 0.0)
                                   ? ms
                                   : webview_backdrop.copy_ms * 0.9 + ms * 0.1;
  }
  /* and how many there really were in the last second */
  now = GetTickCount();
  webview_backdrop.captured++;
  if ((now - webview_backdrop.captured_since) >= 1000) {
    webview_backdrop.fps_real =
        (int)((webview_backdrop.captured * 1000u) /
              (now - webview_backdrop.captured_since));
    webview_backdrop.captured = 0;
    webview_backdrop.captured_since = now;
  }
}

/* A screenshot key went down: be in capture before the tool takes its
   picture, and stay there for a few seconds. */
static void webview_backdrop_reveal(void) {
  struct webview *w = webview_backdrop.w;
  if (!webview_backdrop.on || (w == NULL) ||
      (webview_backdrop.route != WEBVIEW_BACKDROP_SOURCE_EXCLUSION) ||
      (webview_backdrop.policy != WEBVIEW_BACKDROP_SCREENSHOTS)) {
    return;
  }
  webview_backdrop.revealed = 1;
  webview_backdrop_affinity(w, 0);
  SetTimer(w->priv.hwnd, WEBVIEW_BACKDROP_REVEAL_TIMER,
           WEBVIEW_BACKDROP_REVEAL_MS, NULL);
}

static void webview_backdrop_rehide(struct webview *w) {
  KillTimer(w->priv.hwnd, WEBVIEW_BACKDROP_REVEAL_TIMER);
  if (!webview_backdrop.revealed) {
    return;
  }
  webview_backdrop.revealed = 0;
  if (webview_backdrop.on &&
      (webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_EXCLUSION) &&
      (webview_backdrop.policy == WEBVIEW_BACKDROP_SCREENSHOTS)) {
    webview_backdrop_affinity(w, 1);
    webview_backdrop_dwm_flush();
    webview_backdrop_capture(w, 1);
  }
}

static LRESULT CALLBACK webview_backdrop_keys(int code, WPARAM wParam,
                                              LPARAM lParam) {
  if ((code == HC_ACTION) &&
      ((wParam == WM_KEYDOWN) || (wParam == WM_SYSKEYDOWN))) {
    const KBDLLHOOKSTRUCT *key = (const KBDLLHOOKSTRUCT *)lParam;
    int win = ((GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) &
               0x8000) != 0;
    int shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    if ((key->vkCode == VK_SNAPSHOT) ||
        ((key->vkCode == 'S') && win && shift)) {
      webview_backdrop_reveal();
    }
  }
  return CallNextHookEx(NULL, code, wParam, lParam);
}

static void webview_backdrop_hook_keys(int on) {
  if (on && (webview_backdrop.hook == NULL)) {
    webview_backdrop.hook = SetWindowsHookExW(
        WH_KEYBOARD_LL, webview_backdrop_keys, GetModuleHandleW(NULL), 0);
  } else if (!on && (webview_backdrop.hook != NULL)) {
    UnhookWindowsHookEx(webview_backdrop.hook);
    webview_backdrop.hook = NULL;
  }
}

/* Put the window in or out of capture, and the timers and the hook in
   place, for the current policy. */
static void webview_backdrop_apply_policy(struct webview *w) {
  int policy = webview_backdrop.policy;
  if (!webview_backdrop.on || (w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  webview_backdrop.revealed = 0;
  KillTimer(w->priv.hwnd, WEBVIEW_BACKDROP_REVEAL_TIMER);
  if (webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER) {
    /* nothing to hide from: out of capture only if asked */
    webview_backdrop_hook_keys(0);
    webview_backdrop_affinity(w, policy == WEBVIEW_BACKDROP_HIDDEN);
  } else {
    webview_backdrop_hook_keys(policy == WEBVIEW_BACKDROP_SCREENSHOTS);
    webview_backdrop_affinity(w, (policy != WEBVIEW_BACKDROP_VISIBLE) ||
                                     webview_backdrop.sizemove);
  }
  webview_backdrop.fps_now = 0; /* the timer interval depends on it too */
  webview_backdrop_retime(w, GetForegroundWindow() == w->priv.hwnd);
}

WEBVIEW_API void webview_backdrop_set_capture(struct webview *w, int policy) {
  if ((policy < WEBVIEW_BACKDROP_SCREENSHOTS) ||
      (policy > WEBVIEW_BACKDROP_VISIBLE)) {
    policy = WEBVIEW_BACKDROP_SCREENSHOTS;
  }
  webview_backdrop.policy = policy;
  webview_backdrop_apply_policy(w);
  webview_backdrop_capture(w, 1);
}

/* WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE. In VISIBLE, a drag is the one time
   the glass is live: out of capture for the whole drag, back in after. */
static void webview_backdrop_sizemove(struct webview *w, int entering) {
  webview_backdrop.sizemove = entering ? 1 : 0;
  if (!webview_backdrop.on ||
      (webview_backdrop.route != WEBVIEW_BACKDROP_SOURCE_EXCLUSION) ||
      (webview_backdrop.policy != WEBVIEW_BACKDROP_VISIBLE)) {
    return;
  }
  if (entering) {
    webview_backdrop_affinity(w, 1);
  } else {
    webview_backdrop_grab(w, 1);
    webview_backdrop_affinity(w, 0);
  }
}

/* ------------------------------------------------------------ the rate */

/* ------------------------------------------------------------- the pace
 *
 * A Windows timer cannot keep 60 a second, let alone 120: it fires on the
 * system tick, every 15.6 ms, so asking for 16 ms gets 31 - about 32 frames
 * a second. Instead a small thread waits for each refresh of the monitor
 * the window is on (D3DKMTWaitForVerticalBlankEvent, the kernel's vblank
 * wait, in gdi32 since Windows 7 - the one Chromium paces itself with) and
 * posts the window
 * WEBVIEW_BACKDROP_FRAME; the capture itself stays on the window's thread,
 * where the magnifier and WebView2 live. At most one post waits at a time,
 * so a busy window is never flooded.
 *
 * On each refresh webview_backdrop_frame() decides whether to capture:
 * at the rate asked for (focused or behind other windows), never more often
 * than the display refreshes, and always right away after the window moved,
 * so a drag gets every refresh. The rate is measured, not assumed: the
 * display's from the waits, the captures' by counting them.
 *
 * Where the vblank wait is not there or keeps failing (a display that
 * sleeps, a remote session) the thread waits on a high-resolution timer at
 * the display's refresh rate instead - and if it cannot even start, the
 * window's own timer does the work, at 32 a second at best.
 * -------------------------------------------------------------------------*/

typedef struct { /* D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME */
  WCHAR DeviceName[32];
  UINT hAdapter;
  LUID AdapterLuid;
  UINT VidPnSourceId;
} webview_kmt_open;
typedef struct {
  UINT hAdapter;
  UINT hDevice;
  UINT VidPnSourceId;
} webview_kmt_wait;
typedef struct {
  UINT hAdapter;
} webview_kmt_close;
typedef LONG(APIENTRY *webview_kmt_open_fn)(webview_kmt_open *);
typedef LONG(APIENTRY *webview_kmt_wait_fn)(const webview_kmt_wait *);
typedef LONG(APIENTRY *webview_kmt_close_fn)(const webview_kmt_close *);

static struct {
  HANDLE thread;
  volatile LONG stop;
  volatile LONG pending; /* a WEBVIEW_BACKDROP_FRAME is on its way */
  volatile LONG hz100;   /* the display's refresh rate, x100, measured */
  volatile LONG how;     /* 1 the display's vblank, 2 a timer */
  HWND hwnd;
} webview_pacer;

static int webview_pacer_running(void) {
  return webview_pacer.thread != NULL;
}

/* how it waits: 1 the display's refresh, 2 a timer at the display's rate */
static int webview_pacer_how(void) {
  return (int)webview_pacer.how;
}

/* the display's rate in Hz, or 0 if not known yet */
static int webview_pacer_hz(void) {
  return (int)((webview_pacer.hz100 + 50) / 100);
}

/* A precise wait of `seconds` on the pacer's timer (high resolution where
   Windows has it: 1803 and later). */
static void webview_pacer_sleep(HANDLE timer, double seconds) {
  LARGE_INTEGER due;
  if (seconds <= 0.0) {
    return;
  }
  due.QuadPart = -(LONGLONG)(seconds * 10000000.0); /* relative, 100 ns */
  if ((timer != NULL) && SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE)) {
    WaitForSingleObject(timer, 200);
  } else {
    Sleep((DWORD)(seconds * 1000.0) + 1);
  }
}

static DWORD WINAPI webview_pacer_main(LPVOID unused) {
  typedef HANDLE(WINAPI * timer_fn)(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD,
                                    DWORD);
  HMODULE gdi = GetModuleHandleW(L"gdi32.dll");
  HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
  webview_kmt_open_fn open = (gdi != NULL)
      ? (webview_kmt_open_fn)(void *)GetProcAddress(gdi, "D3DKMTOpenAdapterFromGdiDisplayName")
      : NULL;
  webview_kmt_wait_fn wait = (gdi != NULL)
      ? (webview_kmt_wait_fn)(void *)GetProcAddress(gdi, "D3DKMTWaitForVerticalBlankEvent")
      : NULL;
  webview_kmt_close_fn close = (gdi != NULL)
      ? (webview_kmt_close_fn)(void *)GetProcAddress(gdi, "D3DKMTCloseAdapter")
      : NULL;
  timer_fn make_timer = (kernel != NULL)
      ? (timer_fn)(void *)GetProcAddress(kernel, "CreateWaitableTimerExW")
      : NULL;
  HANDLE timer = NULL;
  webview_kmt_open adapter;
  HMONITOR monitor = NULL;
  int have = 0, fails = 0, nominal = 60;
  unsigned n = 0;
  double hz = 0.0;
  LARGE_INTEGER freq, last, now;
  (void)unused;
  if (make_timer != NULL) {
    timer = make_timer(NULL, NULL, 0x2 /* CREATE_WAITABLE_TIMER_HIGH_RESOLUTION */,
                       TIMER_ALL_ACCESS);
  }
  if (timer == NULL) {
    timer = CreateWaitableTimerW(NULL, TRUE, NULL);
  }
  ZeroMemory(&adapter, sizeof(adapter));
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&last);
  while (!webview_pacer.stop) {
    HWND hwnd = webview_pacer.hwnd;
    double period, dt;
    /* follow the window to its monitor, now and then: its refresh rate,
       and its vblank */
    if ((n++ % 32) == 0) {
      HMONITOR m = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
      if ((m != monitor) || !have) {
        MONITORINFOEXW mi;
        DEVMODEW dm;
        if (have && (close != NULL)) {
          webview_kmt_close c;
          c.hAdapter = adapter.hAdapter;
          close(&c);
        }
        have = 0;
        if (m != monitor) {
          hz = 0.0; /* another screen, another rate */
        }
        monitor = m;
        mi.cbSize = sizeof(mi);
        if ((m != NULL) && GetMonitorInfoW(m, (MONITORINFO *)&mi)) {
          ZeroMemory(&dm, sizeof(dm));
          dm.dmSize = sizeof(dm);
          if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) &&
              (dm.dmDisplayFrequency >= 24) && (dm.dmDisplayFrequency <= 500)) {
            nominal = (int)dm.dmDisplayFrequency;
          }
          if ((open != NULL) && (wait != NULL) && (fails < 40)) {
            ZeroMemory(&adapter, sizeof(adapter));
            lstrcpynW(adapter.DeviceName, mi.szDevice, 32);
            have = (open(&adapter) == 0);
          }
        }
      }
    }
    period = 1.0 / ((hz > 0.0) ? hz : (double)nominal);
    if (have) {
      webview_kmt_wait v;
      v.hAdapter = adapter.hAdapter;
      v.hDevice = 0;
      v.VidPnSourceId = adapter.VidPnSourceId;
      if (wait(&v) == 0) {
        webview_pacer.how = 1;
      } else {
        fails++;
        have = 0; /* opened again next round; after many, the timer only */
        n = 0;
        webview_pacer.how = 2;
        webview_pacer_sleep(timer, period);
      }
    } else {
      webview_pacer.how = 2;
      webview_pacer_sleep(timer, period);
    }
    QueryPerformanceCounter(&now);
    dt = (double)(now.QuadPart - last.QuadPart) / (double)freq.QuadPart;
    /* a wait that did not wait (a display going to sleep, a remote
       session): make up the rest of the frame, never spin */
    if (dt < period * 0.6) {
      webview_pacer_sleep(timer, period - dt);
      QueryPerformanceCounter(&now);
      dt = (double)(now.QuadPart - last.QuadPart) / (double)freq.QuadPart;
    }
    last = now;
    /* the display's real rate, from the vblanks */
    if ((webview_pacer.how == 1) && (dt > 0.002) && (dt < 0.1)) {
      hz = (hz <= 0.0) ? 1.0 / dt : hz * 0.95 + (1.0 / dt) * 0.05;
    }
    webview_pacer.hz100 =
        (LONG)(((webview_pacer.how == 1) && (hz > 0.0) ? hz : (double)nominal) *
               100.0);
    if (!IsIconic(hwnd) &&
        (InterlockedCompareExchange(&webview_pacer.pending, 1, 0) == 0) &&
        !PostMessageW(hwnd, WEBVIEW_BACKDROP_FRAME, 0, 0)) {
      webview_pacer.pending = 0;
    }
  }
  if (have && (close != NULL)) {
    webview_kmt_close c;
    c.hAdapter = adapter.hAdapter;
    close(&c);
  }
  if (timer != NULL) {
    CloseHandle(timer);
  }
  return 0;
}

static void webview_pacer_start(HWND hwnd) {
  if (webview_pacer.thread != NULL) {
    webview_pacer.hwnd = hwnd;
    return;
  }
  webview_pacer.stop = 0;
  webview_pacer.pending = 0;
  webview_pacer.hwnd = hwnd;
  webview_pacer.thread = CreateThread(NULL, 0, webview_pacer_main, NULL, 0, NULL);
  if (webview_pacer.thread != NULL) {
    SetThreadPriority(webview_pacer.thread, THREAD_PRIORITY_ABOVE_NORMAL);
  }
}

static void webview_pacer_stop(void) {
  if (webview_pacer.thread == NULL) {
    return;
  }
  webview_pacer.stop = 1;
  /* it wakes on the next refresh; do not hang on a display that sleeps */
  if (WaitForSingleObject(webview_pacer.thread, 200) == WAIT_OBJECT_0) {
    CloseHandle(webview_pacer.thread);
  }
  /* else it is let go: it sees `stop` and ends on its own */
  webview_pacer.thread = NULL;
  webview_pacer.pending = 0;
}

/* Does the timer do the pacing right now, rather than the display? */
static int webview_backdrop_timer_paced(void) {
  return !webview_pacer_running() ||
         ((webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_EXCLUSION) &&
          (webview_backdrop.policy == WEBVIEW_BACKDROP_VISIBLE));
}

/* Set the rate for a focused or a background window - for the display's
   pace, or for the timer when that is what runs. */
static void webview_backdrop_retime(struct webview *w, int active) {
  int fps;
  if (!webview_backdrop.on || (w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  fps = active ? webview_backdrop.fps_focused : webview_backdrop.fps_background;
  if (fps <= 0) {
    fps = webview_backdrop.fps;
  }
  fps = (fps < 1) ? 1 : (fps > 240) ? 240 : fps;
  /* never more than ~40% of the time copying: a slow machine gets fewer
     frames, not a stuttering window */
  if ((webview_backdrop.copy_ms > 0.0) &&
      (fps * webview_backdrop.copy_ms > 400.0)) {
    int most = (int)(400.0 / webview_backdrop.copy_ms);
    fps = (most < 10) ? 10 : (most < fps) ? most : fps;
  }
  if (fps == webview_backdrop.fps_now) {
    return;
  }
  webview_backdrop.fps_now = fps;
  if (!webview_backdrop_timer_paced()) {
    KillTimer(w->priv.hwnd, WEBVIEW_BACKDROP_TIMER);
    return;
  }
  SetTimer(w->priv.hwnd, WEBVIEW_BACKDROP_TIMER,
           ((webview_backdrop.route == WEBVIEW_BACKDROP_SOURCE_EXCLUSION) &&
            (webview_backdrop.policy == WEBVIEW_BACKDROP_VISIBLE))
               ? WEBVIEW_BACKDROP_VISIBLE_MS
               : (UINT)(1000 / fps),
           NULL);
}

/* A faster rate while the window has focus - the one being looked at -
   and a slower one behind other windows. 0 for either uses the fps given
   to webview_backdrop_start(). Callable before or after starting.

     webview_backdrop_set_fps(&w, 240, 30);

   Never more than the display refreshes: 240 means "every refresh" on any
   screen up to 240 Hz. While the window moves, every refresh is captured. */
WEBVIEW_API void webview_backdrop_set_fps(struct webview *w, int focused,
                                          int background) {
  webview_backdrop.fps_focused = (focused < 0) ? 0 : focused;
  webview_backdrop.fps_background = (background < 0) ? 0 : background;
  webview_backdrop.fps_now = 0; /* force the timer to be set again */
  if ((w != NULL) && (w->priv.hwnd != NULL)) {
    webview_backdrop_retime(w, GetForegroundWindow() == w->priv.hwnd);
  }
}

/* ------------------------------------------------------------ the switch */

/* Exclusion: can this window be left out of capture at all? Leaves it out
   if so. Returns 0, or -1 with the reason in the status. */
static int webview_backdrop_try_exclusion(struct webview *w) {
  typedef BOOL(WINAPI * affinity_fn)(HWND, DWORD);
  HMODULE user32 = GetModuleHandleA("user32.dll");
  affinity_fn set_affinity;
  int build = 0;
  webview_windows_version(NULL, NULL, &build);
  if (build < 19041) {
    /* before 2004 the flag acts as WDA_MONITOR: the window would capture
       itself as a black box */
    snprintf(webview_backdrop.status, sizeof(webview_backdrop.status),
             "needs Windows 10 2004 or later (build 19041), this is %d",
             build);
    return -1;
  }
  set_affinity = (user32 != NULL) ? (affinity_fn)(void *)GetProcAddress(
                                        user32, "SetWindowDisplayAffinity")
                                  : NULL;
  /* try the flag once: some windows cannot take it */
  if ((set_affinity == NULL) ||
      !set_affinity(w->priv.hwnd, WEBVIEW_WDA_EXCLUDEFROMCAPTURE)) {
    snprintf(webview_backdrop.status, sizeof(webview_backdrop.status),
             "SetWindowDisplayAffinity failed (error %lu)",
             (unsigned long)GetLastError());
    return -1;
  }
  webview_backdrop.excluded = 1;
  return 0;
}

/* The magnifier: open it and take one test picture of the client area. */
static int webview_backdrop_try_magnifier(struct webview *w) {
  RECT client;
  POINT origin = {0, 0};
  unsigned char *test;
  int ok;
  if (webview_mag_open(w->priv.hwnd, webview_backdrop.route_why,
                       sizeof(webview_backdrop.route_why)) != 0) {
    return -1;
  }
  GetClientRect(w->priv.hwnd, &client);
  ClientToScreen(w->priv.hwnd, &origin);
  if ((client.right < 1) || (client.bottom < 1)) {
    SetRect(&client, 0, 0, 8, 8);
  }
  test = (unsigned char *)malloc((size_t)client.right * client.bottom * 4);
  if (test == NULL) {
    return -1;
  }
  ok = webview_mag_copy(test, origin.x, origin.y, client.right,
                        client.bottom) == 0;
  free(test);
  if (!ok) {
    snprintf(webview_backdrop.route_why, sizeof(webview_backdrop.route_why),
             "it returned no picture");
    webview_mag_close();
    return -1;
  }
  webview_backdrop.route_why[0] = 0;
  webview_backdrop.verified = 0;
  webview_backdrop.verify_tries = 0;
  webview_backdrop.mag_failures = 0;
  webview_backdrop.sent = 0;
  webview_backdrop.verify_at = GetTickCount() + 2000;
  return 0;
}

/* The magnifier failed while running: carry on with exclusion. */
static void webview_backdrop_fall_back(struct webview *w, const char *why) {
  char line[200];
  webview_step("backdrop: the magnifier failed, falling back");
  webview_mag_close();
  snprintf(webview_backdrop.route_why, sizeof(webview_backdrop.route_why),
           "%s", why);
  snprintf(line, sizeof(line), "backdrop: the magnifier failed (%s)%s", why,
           (webview_backdrop.source == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER)
               ? "; stopping"
               : "; using exclusion");
  webview_print_log(line);
  if ((webview_backdrop.source == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER) ||
      (webview_backdrop_try_exclusion(w) != 0)) {
    webview_backdrop_stop(w);
    return;
  }
  webview_backdrop.route = WEBVIEW_BACKDROP_SOURCE_EXCLUSION;
  webview_backdrop_apply_policy(w);
  /* the page may be telling people the window can be recorded: it cannot
     any more. window.webview.on("backdrop", e => e.source) */
  webview_emit(w, "backdrop", "{\"source\":2}");
}

/* Start (or retune) the live backdrop. fps 1-240, scale 1-8 (each frame is
   1/scale the size in each direction), margin 0-200 CSS pixels of desktop
   captured around the window, so a blur near the edge has something to pull
   in instead of fading to black. Returns 0, or -1 with the reason in
   webview_backdrop_status(). */
WEBVIEW_API int webview_backdrop_start(struct webview *w, int fps, int scale,
                                       int margin) {
  if ((w == NULL) || (w->priv.hwnd == NULL)) {
    return -1;
  }
  webview_backdrop.fps = (fps < 1) ? 1 : (fps > 240) ? 240 : fps;
  webview_backdrop.scale = (scale < 1) ? 1 : (scale > 8) ? 8 : scale;
  webview_backdrop.margin = (margin < 0) ? 0 : (margin > 200) ? 200 : margin;
  webview_backdrop.fps_now = 0;
  webview_step("backdrop: start");
  if (webview_backdrop.on) {
    /* already running: new numbers, same route */
    webview_backdrop_retime(w, GetForegroundWindow() == w->priv.hwnd);
    webview_backdrop_capture(w, 1);
    return 0;
  }

  webview_backdrop.route = 0;
  webview_backdrop.route_why[0] = 0;
  webview_backdrop.excluded = 0;
  webview_backdrop.copy_ms = 0.0;
  if (webview_backdrop.source != WEBVIEW_BACKDROP_SOURCE_EXCLUSION) {
    if (webview_backdrop_try_magnifier(w) == 0) {
      webview_backdrop.route = WEBVIEW_BACKDROP_SOURCE_MAGNIFIER;
    } else if (webview_backdrop.source == WEBVIEW_BACKDROP_SOURCE_MAGNIFIER) {
      snprintf(webview_backdrop.status, sizeof(webview_backdrop.status),
               "the magnifier is not available: %s",
               webview_backdrop.route_why);
      webview_print_log(webview_backdrop.status);
      return -1;
    }
  } else {
    snprintf(webview_backdrop.route_why, sizeof(webview_backdrop.route_why),
             "exclusion was asked for");
  }
  if (webview_backdrop.route == 0) {
    if (webview_backdrop_try_exclusion(w) != 0) {
      webview_print_log(webview_backdrop.status);
      return -1;
    }
    webview_backdrop.route = WEBVIEW_BACKDROP_SOURCE_EXCLUSION;
  }

  webview_backdrop.on = 1;
  webview_backdrop.w = w;
  webview_backdrop.status[0] = 0;
  webview_backdrop.captured = 0;
  webview_backdrop.captured_since = GetTickCount();
  webview_backdrop.fps_real = 0;
  webview_backdrop.carry = 0.0;
  webview_pacer_start(w->priv.hwnd);
  webview_print_log(webview_backdrop_status());
  webview_backdrop_apply_policy(w);
  webview_backdrop_capture(w, 1);
  return 0;
}

WEBVIEW_API void webview_backdrop_stop(struct webview *w) {
  typedef BOOL(WINAPI * affinity_fn)(HWND, DWORD);
  HMODULE user32 = GetModuleHandleA("user32.dll");
  affinity_fn set_affinity;
  if (!webview_backdrop.on || (w == NULL) || (w->priv.hwnd == NULL)) {
    return;
  }
  webview_backdrop.on = 0;
  webview_backdrop.route = 0;
  webview_backdrop.fps_now = 0;
  webview_backdrop.revealed = 0;
  webview_backdrop.excluded = 0;
  webview_backdrop_hook_keys(0);
  webview_pacer_stop();
  webview_mag_close();
  KillTimer(w->priv.hwnd, WEBVIEW_BACKDROP_TIMER);
  KillTimer(w->priv.hwnd, WEBVIEW_BACKDROP_REVEAL_TIMER);
  set_affinity = (user32 != NULL) ? (affinity_fn)(void *)GetProcAddress(
                                        user32, "SetWindowDisplayAffinity")
                                  : NULL;
  if (set_affinity != NULL) {
    set_affinity(w->priv.hwnd, WDA_NONE); /* visible to capture again */
  }
  webview_backdrop_post(w, L"wvbd0");
  webview_backdrop_free();
  snprintf(webview_backdrop.status, sizeof(webview_backdrop.status), "off");
}

/* Where the picture comes from - see HOW at the top. AUTO (the default)
   tries the magnifier, then exclusion. Takes effect at once if running. */
WEBVIEW_API void webview_backdrop_set_source(struct webview *w, int source) {
  if ((source < WEBVIEW_BACKDROP_SOURCE_AUTO) ||
      (source > WEBVIEW_BACKDROP_SOURCE_EXCLUSION)) {
    source = WEBVIEW_BACKDROP_SOURCE_AUTO;
  }
  webview_backdrop.source = source;
  if (webview_backdrop.on && (w != NULL)) {
    int fps = webview_backdrop.fps, scale = webview_backdrop.scale;
    int margin = webview_backdrop.margin;
    webview_backdrop_stop(w);
    webview_backdrop_start(w, fps, scale, margin);
  }
}

/* ----------------------------------------------------- wndproc hooks */

static void webview_backdrop_tick(struct webview *w) {
  /* A still desktop for a second: look a third as often until something
     changes. Moves and resizes still capture at once. */
  webview_backdrop.ticks++;
  if ((webview_backdrop.ticks % 60) == 0) {
    /* the copy got slower or faster: the rate follows */
    webview_backdrop_retime(w, GetForegroundWindow() == w->priv.hwnd);
  }
  if ((webview_backdrop.unchanged >= webview_backdrop.fps_now) &&
      ((webview_backdrop.ticks % 3) != 0)) {
    return;
  }
  webview_backdrop_capture(w, 0);
}

/* A refresh of the display: capture now, or let it pass. */
static void webview_backdrop_frame(struct webview *w) {
  double hz;
  int fps;
  webview_pacer.pending = 0;
  if (!webview_backdrop.on || (w == NULL) || webview_backdrop_timer_paced()) {
    return;
  }
  webview_backdrop.ticks++;
  if ((webview_backdrop.ticks % 120) == 0) {
    /* the copy got slower or faster: the rate follows */
    webview_backdrop_retime(w, GetForegroundWindow() == w->priv.hwnd);
  }
  if (webview_backdrop.moved) {
    webview_backdrop_capture(w, 1); /* a drag gets every refresh */
    return;
  }
  /* the rate asked for, out of the display's: owe a capture every
     hz / fps refreshes */
  hz = (double)webview_pacer.hz100 / 100.0;
  if (hz < 20.0) {
    hz = 60.0;
  }
  fps = (webview_backdrop.fps_now > 0) ? webview_backdrop.fps_now : 30;
  webview_backdrop.carry += (double)fps;
  if (webview_backdrop.carry < hz) {
    return;
  }
  webview_backdrop.carry -= hz;
  if (webview_backdrop.carry > hz) {
    webview_backdrop.carry = 0.0;
  }
  /* a still desktop for a second: a third as often until it changes */
  webview_backdrop.due++;
  if ((webview_backdrop.unchanged >= fps) &&
      ((webview_backdrop.due % 3) != 0)) {
    return;
  }
  webview_backdrop_capture(w, 0);
}

/* The window moved or changed size: the picture behind it changed for sure.
   Paced by the display, the next refresh captures it. On the timer, it is
   captured straight away, at most every 16 ms, so a drag follows as closely
   as the screen copy allows. */
static void webview_backdrop_moved(struct webview *w) {
  DWORD now;
  if (!webview_backdrop.on) {
    return;
  }
  if (!webview_backdrop_timer_paced()) {
    webview_backdrop.moved = 1;
    return;
  }
  now = GetTickCount();
  if ((now - webview_backdrop.last_tick) < 16) {
    return;
  }
  webview_backdrop.last_tick = now;
  webview_backdrop_capture(w, 1);
}

/* The window gained or lost focus: switch to the matching rate. */
static void webview_backdrop_activated(struct webview *w, int active) {
  webview_backdrop_retime(w, active);
}

static void webview_backdrop_destroyed(struct webview *w) {
  (void)w;
  webview_backdrop_hook_keys(0);
  webview_pacer_stop();
  webview_mag_close();
  webview_backdrop.w = NULL;
  webview_backdrop.on = 0;
  webview_backdrop.route = 0;
  webview_backdrop_free();
}
