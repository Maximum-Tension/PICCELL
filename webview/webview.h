/* ===========================================================================
 * webview.h - a tiny WebView2 / MSHTML window for C, Windows only.
 *
 * HEADER ONLY. Include it once per program:
 *
 *     #include "webview/webview.h"
 *
 * Everything is `static`, so there is nothing to link and no .c file to add
 * to your build. Include it in exactly one translation unit - the library
 * keeps a little global state (which backend was chosen), and two copies of
 * that state in two .obj files would disagree with each other.
 *
 * The implementation is split into .inl files, included at the bottom in
 * dependency order. Read them in this order to follow the flow:
 *
 *     wv_util.inl       string, GUID and logging helpers
 *     wv_bootstrap.inl  the JavaScript injected into every page
 *     wv_paths.inl      the one data folder (profiles, extracted dll)
 *     wv_mshtml.inl     the old Internet Explorer backend (Windows 7)
 *     wv_loader.inl     finding WebView2Loader.dll
 *     wv_edge.inl       the WebView2 backend
 *     wv_policy.inl     which of the two backends to use
 *     wv_window.inl     the native window, icon and message loop
 *     wv_api.inl        eval, dispatch and the C <-> JavaScript events
 *     wv_guest.inl      the <webview> tag: guest pages, content grabbing
 *     wv_dialog.inl     file and message dialogs
 *     wv_transparent.inl  transparent windows, one call
 *     wv_glass.inl      blurred "glass" windows (acrylic, Mica, blur)
 *     wv_options.inl    w.window.* switches and their setters
 *     wv_backdrop.inl   glass the page draws itself, from a screen copy
 *
 * Optional, define before including:
 *     WEBVIEW2_EMBED_LOADER   load WebView2Loader.dll from our own resources
 *     WEBVIEW2_MEMORY_MODULE  ... and map it without touching the disk
 *                             (also add MemoryModule.c to the build)
 *     WEBVIEW_WIN32_ICON_RES  resource id of the window icon
 *     WEBVIEW_LEGACY_BELOW_MINOR  where "legacy Windows" starts (default 6.2)
 *
 * Based on webview by Serge Zaitsev (MIT), see LICENSE.
 * ===========================================================================
 */

#ifndef WEBVIEW_H
#define WEBVIEW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Header only: every function has internal linkage. */
#define WEBVIEW_API static
#define WEBVIEW_WINAPI 1

#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CINTERFACE
#include <windows.h>

#include <commctrl.h>
#include <exdisp.h>
#include <mshtmhst.h>
#include <mshtml.h>
#include <shellapi.h>
#if defined(__has_include)
#if __has_include(<dwmapi.h>)
#include <dwmapi.h>
#endif
#else
#include <dwmapi.h>
#endif
#include <shobjidl.h>

#define WM_WEBVIEW_DISPATCH (WM_APP + 1)
#define WM_WEBVIEW_READY (WM_APP + 2)

/* ------------------------------------------------------------------------
 * Things older MinGW headers hide behind WINVER / _WIN32_WINNT.
 *
 * We only ever CALL these through GetProcAddress, so declaring them here
 * costs nothing at runtime and does not raise the Windows version your exe
 * needs - it just stops the build from failing on a toolchain whose headers
 * are set to an older target.
 * ------------------------------------------------------------------------ */
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 /* Windows 8.1 */
#endif
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x00080000
#endif
#ifndef LWA_COLORKEY
#define LWA_COLORKEY 0x00000001
#endif
#ifndef LWA_ALPHA
#define LWA_ALPHA 0x00000002
#endif
#ifndef DWM_BB_ENABLE
#define DWM_BB_ENABLE 0x00000001
#define DWM_BB_BLURREGION 0x00000002
#define DWM_BB_TRANSITIONONMAXIMIZED 0x00000004
typedef struct _DWM_BLURBEHIND {
  DWORD dwFlags;
  BOOL fEnable;
  HRGN hRgnBlur;
  BOOL fTransitionOnMaximized;
} DWM_BLURBEHIND;
#endif
/* Sent when DWM restarts or composition is toggled - everything we asked DWM
   for is gone at that point, including the window's alpha channel. It lives
   in dwmapi.h, which this library deliberately does not include (DWM is
   resolved at runtime instead), and some MinGW builds do not surface it
   through windows.h either. */
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif

typedef struct webview2_struct webview2;

/* one C function exposed to the page, see webview_bind() below */
struct webview_binding;

/* one <webview> tag in the page, see wv_guest.inl */
struct webview_guest;

struct webview_priv {
  HWND hwnd;
  IOleObject **browser;
  BOOL is_fullscreen;
  DWORD saved_style;
  DWORD saved_ex_style;
  RECT saved_rect;
  int saved_maximized;    /* was maximized before going fullscreen        */
  int page_fullscreen;    /* the main page has an element in fullscreen   */
  int fullscreen_by_page; /* ...and that is why the window is fullscreen  */
  webview2 *webview2;
  int transparent; /* swallow WM_ERASEBKGND while transparency is on */
  int hidden_frame; /* frameless glass: WS_CAPTION kept, WM_NCCALCSIZE hides
                       the frame - see wv_glass.inl */
  struct webview_binding *bindings;
  int binding_count;
  int binding_capacity;
  struct webview_guest *guests; /* live <webview> tags, see wv_guest.inl */
};

typedef struct webview webview;

typedef char *(*webview_external_invoke_cb_t)(struct webview *w,
                                             const char *arg);

/* ------------------------------------------------------------------------
 * Window settings - one independent toggle each, all defaulting to off.
 * Fill these in before webview_init() and it applies them for you; every
 * one also has a webview_window_set_* function you can call at any time.
 * See wv_options.inl.
 * ---------------------------------------------------------------------- */
/* Tri-state for w.browser.*: 0 leaves WebView2's own default alone. */
#define WEBVIEW_DEFAULT 0
#define WEBVIEW_ON 1
#define WEBVIEW_OFF (-1)

/* ------------------------------------------------------------------------
 * Browser settings - the page-side half of the switches, same shape as
 * w.window but tri-state, so a memset means "WebView2 defaults".
 * See wv_browser.inl.
 * ---------------------------------------------------------------------- */
struct webview_browser_options {
  int context_menu;      /* right-click menu, the one with Inspect on it */
  int devtools;          /* F12 and Inspect; follows w.debug by default  */
  int status_bar;        /* the URL popup in the bottom left corner      */
  int zoom_control;      /* ctrl+wheel and ctrl +/-                      */
  int accelerator_keys;  /* F5, ctrl+P, ctrl+R, ctrl+F ...               */
  int swipe_navigation;  /* touch swipe to go back and forward           */
  int script_dialogs;    /* alert(), confirm(), prompt()                 */
  int error_page;        /* Chromium's own navigation error page         */
  int script;            /* JavaScript itself                            */
  int html_fullscreen;   /* element.requestFullscreen() takes the whole
                            window fullscreen, like a browser does. On
                            unless set to WEBVIEW_OFF, which keeps the
                            element inside the window instead         */
};

struct webview_window_options {
  int transparent;        /* 0 = off, or a WEBVIEW_WINDOW_* mode          */
  int frameless;          /* no title bar and no border                   */
  int always_on_top;      /* stays above other windows                    */
  int click_through;      /* mouse passes straight through to the desktop */
  int hide_from_taskbar;  /* no taskbar button, no alt-tab entry          */
  int center;             /* centre on the screen at startup              */
  int maximized;          /* start maximized                              */
  int fullscreen;         /* start fullscreen                             */
  int opacity;            /* 0-255 uniform opacity; 0 or 255 = opaque     */
  int corner_radius;      /* 0 = Windows decides, -1 = square, or pixels
                             at 100% scaling - see wv_options.inl         */
  int webview_loaded;     /* set by the library when the page reports in;
                             read it, do not set it                        */
};

struct webview {
  const char *url;
  const char *title;
  int width;
  int height;
  int resizable;
  int debug;
  webview_external_invoke_cb_t external_invoke_cb;
  struct webview_window_options window;
  struct webview_browser_options browser;
  struct webview_priv priv;
  void *userdata;
};

enum webview_dialog_type {
  WEBVIEW_DIALOG_TYPE_OPEN = 0,
  WEBVIEW_DIALOG_TYPE_SAVE = 1,
  WEBVIEW_DIALOG_TYPE_ALERT = 2
};

enum webview_dialog_flag {
  WEBVIEW_DIALOG_FLAG_FILE = (0 << 0),
  WEBVIEW_DIALOG_FLAG_DIRECTORY = (1 << 0),

  WEBVIEW_DIALOG_FLAG_INFO = (1 << 1),
  WEBVIEW_DIALOG_FLAG_WARNING = (2 << 1),
  WEBVIEW_DIALOG_FLAG_ERROR = (3 << 1),
  WEBVIEW_DIALOG_FLAG_ALERT_MASK = (3 << 1),
};

typedef void (*webview_dispatch_fn)(struct webview *w, void *arg);

typedef struct webview_dispatch_arg webview_dispatch_arg;

struct webview_dispatch_arg {
  webview_dispatch_fn fn;
  struct webview *w;
  void *arg;
};

WEBVIEW_API int webview_run(const char *title, const char *url, int width,
                        int height, int resizable);

WEBVIEW_API int webview_init(struct webview *w);
WEBVIEW_API int webview_loop(struct webview *w, int blocking);
WEBVIEW_API int webview_eval(struct webview *w, const char *js);
WEBVIEW_API int webview_set_html(struct webview *w, const char *html);
WEBVIEW_API int webview_inject_css(struct webview *w, const char *css);
WEBVIEW_API void webview_set_title(struct webview *w, const char *title);
WEBVIEW_API void webview_set_fullscreen(struct webview *w, int fullscreen);
WEBVIEW_API void webview_set_color(struct webview *w, uint8_t r, uint8_t g,
                                   uint8_t b, uint8_t a);
WEBVIEW_API void webview_dialog(struct webview *w,
                                enum webview_dialog_type dlgtype, int flags,
                                const char *title, const char *arg,
                                char *result, size_t resultsz);
WEBVIEW_API void webview_dispatch(struct webview *w, webview_dispatch_fn fn,
                                  void *arg);
WEBVIEW_API void webview_terminate(struct webview *w);
WEBVIEW_API void webview_exit(struct webview *w);
WEBVIEW_API void webview_debug(const char *format, ...);
WEBVIEW_API void webview_print_log(const char *s);
/* Debugging aids, both off unless called. A name alone goes next to the
   exe. log_file: every log line, also in a file. crash_file: if the
   process dies of an unhandled exception, a note of what, where, the call
   stack and the library's last steps. */
WEBVIEW_API int webview_log_file(const char *name);
WEBVIEW_API int webview_crash_file(const char *name);

/* Extra command line switches for the browser process, e.g. "--disable-gpu".
   Must be called BEFORE webview_init(). */
WEBVIEW_API int webview_set_browser_arguments(const char *args);

/* Ask for a transparent background BEFORE the browser is created.
   Call this before webview_init().

   webview_set_transparent() can only run once the window exists, and that is
   already too late for the browser's FIRST painted frame: that frame uses the
   background colour decided when the WebView2 environment was created, and
   the default is opaque white. That is the "transparent for half a second,
   then white" flicker exactly.

   This sets WEBVIEW2_DEFAULT_BACKGROUND_COLOR, a documented WebView2
   environment variable (hex AARRGGBB), so the first frame is transparent too.
   You still want webview_set_transparent() after init, for the window side. */
WEBVIEW_API void webview_prefer_transparent(int transparent);

/* file:/// url for a file next to the exe, e.g. webview_local_url(buf, sizeof
   buf, "data/index.html"). */
WEBVIEW_API char *webview_local_url(char *buffer, size_t size,
                                    const char *relative);

/* ------------------------------------------------------------------------
 * Binding C functions into the page
 *
 * Instead of one external_invoke_cb receiving every message as a string, you
 * can expose named functions that JavaScript calls directly:
 *
 *     webview_bind(&w, "app.readFile", &read_file, NULL);
 *
 *     const text = await app.readFile("notes.txt");
 *
 * Dotted names create the namespace for you. The JS side always gets a
 * Promise, because the answer has to travel back through the message loop -
 * so a binding can take as long as it likes without freezing the page.
 *
 * Your handler receives the arguments as a JSON array and must answer exactly
 * once with webview_return(), now or later:
 *
 *     static void read_file(struct webview *w, const char *seq,
 *                           const char *args, void *userdata) {
 *       webview_return(w, seq, 0, "\"file contents\"");   resolve
 *       webview_return(w, seq, 1, "\"no such file\"");    reject
 *     }
 *
 * Answering from another thread is fine: webview_return is safe to call
 * anywhere, it hops onto the UI thread itself.
 * ------------------------------------------------------------------------ */

/* status 0 resolves the promise, anything else rejects it. */
typedef void (*webview_bind_fn)(struct webview *w, const char *seq,
                                const char *args_json, void *userdata);

struct webview_binding {
  char *name;
  webview_bind_fn fn;
  void *userdata;
};

WEBVIEW_API int webview_bind(struct webview *w, const char *name,
                             webview_bind_fn fn, void *userdata);
WEBVIEW_API int webview_unbind(struct webview *w, const char *name);
WEBVIEW_API int webview_return(struct webview *w, const char *seq, int status,
                               const char *result_json);

/* internal: (re)install every binding's JS stub, handle an incoming call,
   and run a script on the UI thread */
WEBVIEW_API void webview_install_bindings(struct webview *w);
static int webview_dispatch_binding(struct webview *w, const char *message);
static void webview_apply_window_options(struct webview *w);
static void webview_apply_page_options(struct webview *w);
static void webview_refresh_transparency(struct webview *w);
static int webview_erase_armed;
static void webview_erase_transparent(struct webview *w, HDC dc);
static int webview_retry_transparency(struct webview *w);
#define WEBVIEW_TRANSPARENCY_TIMER 0xB0B0
#define WEBVIEW_ERASE_TIMER 0xB0B2

/* internal: corner radius (wv_options.inl) */
static void webview_apply_corners(struct webview *w);

/* internal: the self-drawn glass backdrop (wv_backdrop.inl) */
#define WEBVIEW_BACKDROP_TIMER 0xB0B6
#define WEBVIEW_BACKDROP_REVEAL_TIMER 0xB0B8
#define WEBVIEW_BACKDROP_FRAME (WM_APP + 0x0B0B) /* a display refresh */
static void webview_backdrop_rehide(struct webview *w);
static void webview_backdrop_sizemove(struct webview *w, int entering);
static void webview_backdrop_tick(struct webview *w);
static void webview_backdrop_moved(struct webview *w);
static void webview_backdrop_destroyed(struct webview *w);
static void webview_backdrop_activated(struct webview *w, int active);
static void webview_backdrop_retime(struct webview *w, int active);
static void webview_backdrop_frame(struct webview *w);
static void webview_backdrop_page_takes_shared(void);

/* internal: blurred backdrops (wv_glass.inl) */
static int webview_mode_is_glass(int mode);
static int webview_glass_apply(struct webview *w, int mode);
static void webview_glass_clear(struct webview *w, int mode);
static int webview_glass_keep_active_now(struct webview *w);
static void webview_glass_settings_changed(struct webview *w, UINT msg,
                                           LPARAM lParam);
static int webview_glass_nccalcsize(struct webview *w, HWND hwnd,
                                    WPARAM wParam, LPARAM lParam,
                                    LRESULT *result);
WEBVIEW_API int webview_apply_browser_options(struct webview *w);
static void webview_eval_dispatched(struct webview *w, void *arg);
static char *webview_strdup(const char *s);

/* internal: the <webview> tag (wv_guest.inl) */
#define WEBVIEW_GUEST_DRAG_TIMER 0xB0B4
static int webview_guest_on_host_message(struct webview *w, const char *msg);
static void webview_guests_close_all(struct webview *w);
static void webview_guests_parent_moved(struct webview *w);
static void webview_guest_drag_tick(struct webview *w);
static void webview_guest_install_host_script(webview2 *pwv2);
static void webview_guests_parent_resized(struct webview *w);
static int webview_guests_any_fullscreen(struct webview *w);
static void webview_guests_exit_fullscreen(struct webview *w);

/* internal: a page (the main one or a <webview> guest) entered or left
   HTML fullscreen - see wv_options.inl */
static void webview_html_fullscreen_changed(struct webview *w);
static int webview_html_fullscreen_allowed(struct webview *w);

/* ------------------------------------------------------------------------
 * C -> JavaScript messaging (added)
 * ------------------------------------------------------------------------ */

/* Quote and escape a UTF-8 string as a JSON string literal.
   Returns a malloc'ed buffer (including the surrounding double quotes)
   that the caller must free(), or NULL on allocation failure. */
WEBVIEW_API char *webview_json_escape(const char *s);

/* Deliver an event to the page. `json` must be a valid JSON value
   (object, array, string, number, true/false/null) or NULL for null.
   On the page: window.webview.on("name", function (data) { ... }) */
WEBVIEW_API int webview_emit(struct webview *w, const char *name,
                             const char *json);

/* Same, but `value` is a plain string that is escaped for you. */
WEBVIEW_API int webview_emit_str(struct webview *w, const char *name,
                                 const char *value);

/* Thread-safe version of webview_emit: may be called from any thread,
   the event is delivered on the UI thread. */
WEBVIEW_API int webview_emit_async(struct webview *w, const char *name,
                                   const char *json);

/* Callback receiving the result of an evaluated script, as JSON.
   `json` is "null" when the script failed or returned nothing. */
typedef void (*webview_eval_cb_t)(struct webview *w, const char *json,
                                  void *userdata);

/* Evaluate JavaScript and get the result back in C (WebView2 only,
   returns -1 on the IE fallback). The callback runs on the UI thread. */
WEBVIEW_API int webview_eval_result(struct webview *w, const char *js,
                                    webview_eval_cb_t cb, void *userdata);

/* ------------------------------------------------------------------------
 * Data folder (wv_paths.inl)
 *
 * Everything lives under %LOCALAPPDATA%\webview-c\ :
 *   runtime\             the extracted loader dll (not with MEMLOAD=1)
 *   projects\<project>\  cookies, storage and cache, one folder per project
 * ---------------------------------------------------------------------- */

/* The project name. Every exe using the same name shares one profile.
   NULL = the exe name without ".exe". Before webview_init(). */
WEBVIEW_API void webview_set_app_name(const char *name);

/* Move the whole root (UTF-8). NULL = %LOCALAPPDATA%\webview-c. Before
   webview_init(). The WEBVIEW_DATA_ROOT environment variable does the same. */
WEBVIEW_API void webview_set_data_root(const char *path);

/* <root>\projects\<project>\<sub>, created. For your own files. */
WEBVIEW_API char *webview_data_dir(char *buffer, size_t size, const char *sub);

/* Delete a project's data (NULL = this one). Only while no browser uses it. */
WEBVIEW_API int webview_data_clear(const char *project);

/* ------------------------------------------------------------------------
 * The <webview> tag (wv_guest.inl) - mostly driven from the page:
 *
 *     <webview src="https://example.com"><p>fallback</p></webview>
 *
 * Every tag gets a numeric id, readable as el.guestId, which these take.
 * ---------------------------------------------------------------------- */

/* Run a script in a guest page and get its result as JSON. */
WEBVIEW_API int webview_guest_eval(struct webview *w, int guest_id,
                                   const char *js, webview_eval_cb_t cb,
                                   void *userdata);

/* Point a guest somewhere else. */
WEBVIEW_API int webview_guest_navigate(struct webview *w, int guest_id,
                                       const char *url);

/* Let pages from this origin use the <webview> tag, e.g.
   "https://app.example.com". Local pages (file://, about:, data:, localhost)
   always may; any other page - say a remote site shown in the main window -
   may not, because a guest can open any site and read it back. "*" allows
   every origin. */
WEBVIEW_API int webview_guest_allow_origin(const char *origin);

#if defined(WEBVIEW_WINAPI)
/* 1 when the window is backed by WebView2 (Chromium), 0 when it fell back
   to the old MSHTML/IE control. Valid after webview_init(). */
WEBVIEW_API int webview_backend(struct webview *w);

/* Human readable explanation of the backend choice - useful to find out
   why WebView2 was not picked on a user's machine. */
WEBVIEW_API const char *webview_backend_info(void);

/* Which backend webview_init() should pick. Set BEFORE webview_init(). */
enum webview_backend_policy {
  /* Legacy Windows (7 and older): WebView2 if a runtime happens to be
     there, otherwise MSHTML/IE, silently, no message box.
     Windows 8 and newer: WebView2 required, message box if missing.
     This is the default. */
  WEBVIEW_BACKEND_AUTO = 0,
  /* WebView2 or nothing: message box + webview_init() returns -2. */
  WEBVIEW_BACKEND_WEBVIEW2 = 1,
  /* WebView2 first, silent MSHTML/IE fallback on any Windows version. */
  WEBVIEW_BACKEND_ALLOW_IE = 2,
  /* Never touch WebView2, go straight to MSHTML/IE. */
  WEBVIEW_BACKEND_IE = 3
};

WEBVIEW_API void webview_set_backend_policy(int policy);

/* Shorthand for WEBVIEW_BACKEND_ALLOW_IE / WEBVIEW_BACKEND_WEBVIEW2. */
WEBVIEW_API void webview_set_ie_fallback(int allow);

/* 1 on Windows 7 or older (see WEBVIEW_LEGACY_BELOW_* to move the line). */
WEBVIEW_API int webview_is_legacy_windows(void);

/* Modern JavaScript on the IE fallback (wv_legacy.inl): pages, .js files,
   webview_set_html() and webview_eval() are translated to ES3 and get the
   runtime they need. On by default; 0 turns it off. */
WEBVIEW_API void webview_set_legacy_js(int enabled);

/* Opens the devtools now: WebView2's own window, or on the IE backend a
   console and element inspector drawn into the page (F12 opens it there
   too, when w.browser.devtools or w.debug is on). */
WEBVIEW_API void webview_open_devtools(struct webview *w);

/* The translator itself: modern JavaScript in, ES3 out (malloc'd, free it).
   A script with nothing newer than ES3 comes back unchanged. NULL on a
   syntax error or a feature it cannot translate, with the reason in err.
   The output calls helpers from webview/js/legacy_runtime.js. */
WEBVIEW_API char *webview_downlevel(const char *src, size_t len, char *err,
                                    size_t errlen);

/* WEBVIEW_DOWNLEVEL_OLD_IE: translate for IE8 and older, which have no
   getters or setters on plain objects (and IE6/7 no Element.prototype):
   o.name goes through __wv.g / __wv.s / __wv.d when name is a getter or
   setter some script declared, or an element property or method IE6/7
   lack. The IE backend turns it on by itself when it finds IE8 or older. */
#define WEBVIEW_DOWNLEVEL_OLD_IE 1
WEBVIEW_API void webview_downlevel_mode(int flags);

/* Real OS version, read through RtlGetVersion so the manifest cannot lie
   about it. Any pointer may be NULL. */
WEBVIEW_API void webview_windows_version(int *major, int *minor, int *build);

/* Drop the title bar and border. You lose dragging, resizing and snap with
   it - handle WM_NCHITTEST yourself if you want them back. */
WEBVIEW_API void webview_set_frameless(struct webview *w, int frameless);

/* Give the window a real alpha channel and make WebView2 paint with a
   transparent background, so the page composites onto whatever is behind it.
   Your CSS has to cooperate:  html, body { background: transparent; }
   Returns 0 on success, -1 if the runtime is too old (needs 1.0.774.44) or
   the MSHTML fallback is in use. */
WEBVIEW_API int webview_set_transparent(struct webview *w, int transparent);

/* Frosted glass behind the page (Windows 10/11 acrylic) instead of the bare
   desktop. Same requirements as webview_set_transparent. */
WEBVIEW_API int webview_set_acrylic(struct webview *w, int enabled);

/* Glass - the desktop behind the window, blurred by Windows. Pick it with
   w.window.transparent = WEBVIEW_WINDOW_GLASS (or _MICA, _MICA_ALT, _BLUR);
   these tune it. See wv_glass.inl and examples/10_glass.
     webview_glass_tuning   -1 leaves a switch alone: layered prime,
                            blur-behind alpha, extend the frame over the
                            client area, keep the material while inactive
     webview_set_glass_dark dark or light variant of the material
     webview_glass_info     which API drew it, and what it returned */
WEBVIEW_API void webview_glass_tuning(int layered, int blur_behind,
                                      int extend_frame, int keep_active);
WEBVIEW_API void webview_set_glass_dark(int dark);
WEBVIEW_API const char *webview_glass_info(void);
/* Why Windows will not draw any material right now - transparency effects
   off, energy saver, Remote Desktop - or NULL if it will. The page also gets
   window.webview.on("glass", e => e.blocked) when that changes. */
WEBVIEW_API const char *webview_glass_blocked(void);

/* Glass the page draws itself: a live picture of the desktop behind the
   window, posted to the page, so it can blur at any strength and bend it
   with SVG filters. Taken with the Magnification API, which leaves the
   window out of that picture only - it stays in screenshots, recordings and
   screen sharing. Where that is not available, the window is excluded from
   all capture instead (Windows 10 2004+). See wv_backdrop.inl and the
   page-drawn glass in examples/10_glass. */
WEBVIEW_API int webview_backdrop_start(struct webview *w, int fps, int scale,
                                       int margin);
WEBVIEW_API void webview_backdrop_stop(struct webview *w);
/* Where the picture comes from. AUTO, the default, is the magnifier and
   exclusion only if the magnifier cannot. webview_backdrop_source() says
   which one is running (0 when off). */
#define WEBVIEW_BACKDROP_SOURCE_AUTO 0
#define WEBVIEW_BACKDROP_SOURCE_MAGNIFIER 1 /* in every capture, live     */
#define WEBVIEW_BACKDROP_SOURCE_EXCLUSION 2 /* out of capture, see below  */
WEBVIEW_API void webview_backdrop_set_source(struct webview *w, int source);
WEBVIEW_API int webview_backdrop_source(void);
/* Who can see the window in screenshots and screen sharing while the
   backdrop runs - see "who can see it" in wv_backdrop.inl. With the
   magnifier, only HIDDEN changes anything. */
#define WEBVIEW_BACKDROP_SCREENSHOTS 0 /* exclusion: screenshot keys bring
                                          it back                         */
#define WEBVIEW_BACKDROP_HIDDEN 1      /* never in any capture            */
#define WEBVIEW_BACKDROP_VISIBLE 2     /* exclusion: always in capture;
                                          picture refreshed every 2 s     */
WEBVIEW_API void webview_backdrop_set_capture(struct webview *w, int policy);
/* e.g. 60 while focused, 30 behind other windows; 0 = the start() fps */
WEBVIEW_API void webview_backdrop_set_fps(struct webview *w, int focused,
                                          int background);
WEBVIEW_API const char *webview_backdrop_status(void);

/* Transparency without needing a window alpha channel: a layered window
   punches out every pixel of exactly this colour, and WebView2 is told to
   paint it wherever the page paints nothing. Works where the alpha routes
   do not, at the cost of hard edges. Pick a colour you never draw. */
WEBVIEW_API int webview_set_transparent_colorkey(struct webview *w, uint8_t r,
                                                 uint8_t g, uint8_t b);

/* Colour WebView2 paints where the page paints nothing. Alpha 0 goes with
   webview_set_transparent(); a solid colour is the quickest way to prove the
   call is reaching the runtime. Needs runtime 1.0.774.44 or newer. */
WEBVIEW_API int webview_set_background(struct webview *w, uint8_t r, uint8_t g,
                                       uint8_t b, uint8_t a);

/* Replace the window icon at runtime with an .ico file. */
WEBVIEW_API int webview_set_icon(struct webview *w, const char *icoPath);

/* Take the icon out of an .exe or .dll file instead. Pass NULL for this
   application's own exe, and `index` 0 for its first icon group. */
WEBVIEW_API int webview_set_icon_from_module(struct webview *w,
                                             const char *modulePath,
                                             int index);
#endif

/* Set by webview_init(): 1 = WebView2, 0 = the MSHTML/IE fallback. */
static int webview_webview2_enabled = 0;

/* --------------------------------------------------------------------------
 * Implementation
 * ------------------------------------------------------------------------*/

#include "wv_util.inl"
#include "wv_bootstrap.inl"
#include "wv_paths.inl"
#include "wv_downlevel.inl"
#include "wv_legacy.inl"
#include "wv_mshtml.inl"

#include "ms.webview2/include/WebView2.h"
#if !defined(__WebView2_LIBRARY_DEFINED__)
#error "WebView2.h was not usable - check the include path"
#endif

#include "wv_loader.inl"
#include "wv_edge.inl"
#include "wv_policy.inl"
#include "wv_window.inl"
#include "wv_api.inl"
#include "wv_guest.inl"
#include "wv_dialog.inl"
#include "wv_transparent.inl"
#include "wv_glass.inl"
#include "wv_options.inl"
#include "wv_backdrop.inl"
#include "wv_browser.inl"

#ifdef __cplusplus
}
#endif

#endif /* WEBVIEW_H */
