/* ---------------------------------------------------------------------------
 * wv_guest.inl - the <webview> tag
 *
 *     <webview src="https://example.com" partition="example">
 *       <p>Shown if the page cannot be loaded.</p>
 *     </webview>
 *
 * WHAT IT IS
 *
 * Not an iframe, and not the page's HTML pasted into yours. Every <webview>
 * is a real second browser page - its own WebView2 controller, in its own
 * renderer process - placed exactly over the element's rectangle and kept
 * there as the host page scrolls, resizes and re-lays out. So the guest page
 * gets everything a normal tab gets, for free, because it IS a normal tab:
 * its own window/document/location/history, cookies, localStorage,
 * IndexedDB, fetch/XHR, WebSockets, service workers. Nothing is emulated.
 *
 * ISOLATION
 *
 *   guest -> host   impossible: different process, different document, and
 *                   none of your webview_bind() functions exist over there.
 *                   The one door is chrome.webview.postMessage, which arrives
 *                   on the element as an 'ipc-message' event and nothing else.
 *   host  -> guest  el.executeScript(), el.insertCSS(), el.addInitScript(),
 *                   el.send(), el.grab() - all routed through C.
 *
 * STORAGE
 *
 * Each `partition` is a WebView2 profile, stored inside this project's data
 * folder (see wv_paths.inl):
 *
 *     projects\<project>\EBWebView\<partition>\
 *
 * No partition attribute = one derived from the src host name, so every site
 * keeps its own jar. `incognito` = in-memory only, gone when closed.
 *
 * HOW THE PIECES TALK
 *
 *   host page JS  --"__wvg\x1f<op>\x1f<id>\x1f<req>\x1f<args...>"-->  C
 *   C             --PostWebMessageAsJson {"__wvg":...}------------->  host
 *   guest agent   --"__wvga\x1f<type>\x1f<json>"------------------->  C
 *   guest page    --anything else-----------------------> C --> 'ipc-message'
 *
 * The host side of the tag is js/guest_host.js, the in-guest helper (content
 * grabbing, the element picker, opacity) is js/guest_agent.js. Both are
 * compiled in from wv_guest_js.h, which build.bat regenerates every build.
 *
 * STACKING, OPACITY, TRANSPARENCY
 *
 * The guest is a separate native surface on top of your page, so the
 * browser cannot paint your DOM over it by itself. guest_host.js works out
 * CSS paint order instead - stacking contexts, z-index, positioning, tree
 * order, the top layer - and every element of yours that CSS paints above
 * the <webview> cuts a hole in the guest window (SetWindowRgn, see
 * wvg_apply_shape) and shows through, clicks included. Several <webview>s
 * are stacked in the same order ("order" op, wvg_apply_order).
 *
 * CSS opacity on the element and its ancestors, and the transparent
 * attribute, are handed to the guest (the "look" op): its default
 * background becomes transparent, the agent fades the page itself, and
 * Windows composites the result over your page.
 *
 * LIMITS, HONESTLY
 *
 * CSS transforms, filters and border-radius on the element do not reach the
 * guest. For those moments call el.freeze(): the element shows a still image
 * of the page and the live surface steps aside until el.unfreeze(). Under a
 * faded guest, a hole shows what is behind the webview, not the faded page.
 * Clipping by scrolling containers IS handled - the guest sits inside a child
 * window sized to the visible part of the element.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#include "wv_guest_js.h"

#define WVG_PREFIX "__wvg\x1f"
#define WVG_AGENT_PREFIX "__wvga\x1f"
#define WVG_FRAME_CLASS L"webview_guest_frame"

enum {
  WVG_TOK_NAV_STARTING = 0,
  WVG_TOK_NAV_COMPLETED,
  WVG_TOK_TITLE,
  WVG_TOK_SOURCE,
  WVG_TOK_HISTORY,
  WVG_TOK_NEW_WINDOW,
  WVG_TOK_MESSAGE,
  WVG_TOK_PROCESS_FAILED,
  WVG_TOK_FULLSCREEN,
  WVG_TOK_COUNT
};

struct webview_guest {
  LONG ref;
  struct webview *owner;
  int id;
  int closed;
  int popups;      /* let window.open() make real popup windows */
  int incognito;
  int failed;      /* last navigation failed: show the host's fallback */
  int loaded_once; /* stays hidden until the first successful load */
  int frozen;      /* el.freeze(): surface hidden, snapshot shown */
  int fullscreen;  /* the guest page put an element in fullscreen, so the
                      guest covers the whole window - see wvg_on_fullscreen */
  char *src;
  WCHAR profile[65];
  HWND frame;      /* child window of the host, clips the guest */
  /* last bounds from the host, physical pixels relative to the client area */
  int x, y, w, h;          /* the element's box */
  int cx, cy, cw, ch;      /* the visible (clipped) part of it */
  int visible;             /* the element is rendered at all */
  /* holes: host elements marked data-over-webview that sit on top of the
     guest. Each is x,y,w,h,radius, relative to the frame, physical pixels. */
  int (*holes)[5];
  int hole_count;
  int hole_capacity;
  int shaped;              /* a window region is currently set */
  ICoreWebView2Controller *controller;
  ICoreWebView2 *view;
  EventRegistrationToken tok[WVG_TOK_COUNT];
  int has_tok[WVG_TOK_COUNT];
  EventRegistrationToken accel_tok; /* Esc out of fullscreen, on the controller */
  int has_accel_tok;
  char **scripts; /* addInitScript() calls made before the page existed */
  int script_count;
  /* the element's look, from the host's CSS - see "Look" below */
  int look_opacity;     /* 0..1000, the element's effective opacity */
  int look_mode;        /* 0 opaque, 1 transparent, 2 transparent="force" */
  int look_clear;       /* default background currently transparent */
  WCHAR *look_script;   /* id of the document-created script that sets it */
  int look_busy;        /* that script is being added */
  int look_dirty;       /* ...and the look changed meanwhile */
  struct webview_guest *next;
};

/* --------------------------------------------------------------------------
 * A small generic COM object.
 *
 * Every WebView2 handler interface has the same shape - IUnknown plus one
 * Invoke - so one struct serves them all; only the vtbl (and so the Invoke
 * signature) differs. Reference counted for real, and each object holds a
 * reference on its guest, so a guest is only freed once WebView2 has let go
 * of every handler that could still point at it.
 * ------------------------------------------------------------------------*/
typedef struct wvg_obj wvg_obj;
typedef struct {
  HRESULT(STDMETHODCALLTYPE *QueryInterface)(wvg_obj *, REFIID, void **);
  ULONG(STDMETHODCALLTYPE *AddRef)(wvg_obj *);
  ULONG(STDMETHODCALLTYPE *Release)(wvg_obj *);
  void *Invoke;
  const IID *iid;
} wvg_vtbl;

struct wvg_obj {
  const wvg_vtbl *lpVtbl;
  LONG ref;
  struct webview *w; /* for handlers that belong to the host, not a guest */
  struct webview_guest *g;
  int req;         /* host request id to settle, for one-shot handlers */
  IStream *stream; /* capturePage() */
  int jpeg;        /* ...and in which format */
};

static void webview_guest_release(struct webview_guest *g);

static HRESULT STDMETHODCALLTYPE wvg_obj_qi(wvg_obj *This, REFIID riid,
                                            void **out) {
  if (out == NULL) {
    return E_POINTER;
  }
  if (iid_eq(riid, &IID_IUnknown) || iid_eq(riid, This->lpVtbl->iid)) {
    *out = This;
    InterlockedIncrement(&This->ref);
    return S_OK;
  }
  *out = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE wvg_obj_addref(wvg_obj *This) {
  return (ULONG)InterlockedIncrement(&This->ref);
}

static ULONG STDMETHODCALLTYPE wvg_obj_release(wvg_obj *This) {
  LONG n = InterlockedDecrement(&This->ref);
  if (n > 0) {
    return (ULONG)n;
  }
  if (This->stream != NULL) {
    This->stream->lpVtbl->Release(This->stream);
  }
  if (This->g != NULL) {
    webview_guest_release(This->g);
  }
  free(This);
  return 0;
}

static wvg_obj *wvg_obj_new(const wvg_vtbl *vtbl, struct webview_guest *g,
                            int req) {
  wvg_obj *o = (wvg_obj *)calloc(1, sizeof(wvg_obj));
  if (o == NULL) {
    return NULL;
  }
  o->lpVtbl = vtbl;
  o->ref = 1;
  o->g = g;
  o->req = req;
  if (g != NULL) {
    InterlockedIncrement(&g->ref);
  }
  return o;
}

#define WVG_VTBL(name, invoke, iidp)                                           \
  static const wvg_vtbl name = {&wvg_obj_qi, &wvg_obj_addref,                  \
                                &wvg_obj_release, (void *)(invoke), (iidp)}

/* --------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------*/

/* Send a JSON message to the host page. */
static void wvg_host_post(struct webview *w, const char *json) {
  WCHAR *wjson;
  if ((w == NULL) || (w->priv.webview2 == NULL) ||
      (w->priv.webview2->webview == NULL) || (json == NULL)) {
    return;
  }
  wjson = webview_to_utf16(json);
  if (wjson == NULL) {
    return;
  }
  w->priv.webview2->webview->lpVtbl->PostWebMessageAsJson(
      w->priv.webview2->webview, wjson);
  GlobalFree(wjson);
}

/* {"__wvg":"ev","id":<id>,"type":"<type>","detail":<detail>} */
static void wvg_emit(struct webview_guest *g, const char *type,
                     const char *detail_json) {
  char *msg;
  size_t n;
  if ((g == NULL) || g->closed) {
    return;
  }
  if (detail_json == NULL) {
    detail_json = "{}";
  }
  n = strlen(type) + strlen(detail_json) + 80;
  msg = (char *)malloc(n);
  if (msg == NULL) {
    return;
  }
  snprintf(msg, n, "{\"__wvg\":\"ev\",\"id\":%d,\"type\":\"%s\",\"detail\":%s}",
           g->id, type, detail_json);
  wvg_host_post(g->owner, msg);
  free(msg);
}

/* {"__wvg":"settle","req":<req>,"ok":<ok>,"value":<json>} */
static void wvg_settle(struct webview *w, int req, int ok,
                       const char *value_json) {
  char *msg;
  size_t n;
  if (req <= 0) {
    return;
  }
  if (value_json == NULL) {
    value_json = "null";
  }
  n = strlen(value_json) + 80;
  msg = (char *)malloc(n);
  if (msg == NULL) {
    return;
  }
  snprintf(msg, n, "{\"__wvg\":\"settle\",\"req\":%d,\"ok\":%s,\"value\":%s}",
           req, ok ? "true" : "false", value_json);
  wvg_host_post(w, msg);
  free(msg);
}

static void wvg_settle_str(struct webview *w, int req, int ok,
                           const char *text) {
  char *esc = webview_json_escape(text);
  wvg_settle(w, req, ok, esc != NULL ? esc : "null");
  free(esc);
}

/* A UTF-16 string from WebView2 as an escaped JSON string (malloc'ed). The
   CoTaskMem string is freed. */
static char *wvg_take_wstr_json(LPWSTR ws) {
  char *utf8;
  char *esc;
  if (ws == NULL) {
    return webview_json_escape("");
  }
  utf8 = webview_from_utf16(ws);
  CoTaskMemFree(ws);
  esc = webview_json_escape(utf8 != NULL ? utf8 : "");
  if (utf8 != NULL) {
    GlobalFree(utf8);
  }
  return esc;
}

static struct webview_guest *wvg_find(struct webview *w, int id) {
  struct webview_guest *g;
  for (g = (w != NULL) ? w->priv.guests : NULL; g != NULL; g = g->next) {
    if ((g->id == id) && !g->closed) {
      return g;
    }
  }
  return NULL;
}

/* Profile names may only use a small alphabet and 64 characters. */
static void wvg_profile_name(WCHAR *out, const char *partition,
                             const char *src) {
  char tmp[65];
  const char *p;
  size_t i = 0;
  if ((partition != NULL) && (partition[0] != '\0')) {
    p = partition;
  } else if (src != NULL) {
    /* no partition: one per host name, so every site has its own jar */
    p = strstr(src, "://");
    p = (p != NULL) ? p + 3 : src;
    if (strncmp(p, "www.", 4) == 0) {
      p += 4;
    }
  } else {
    p = "guest";
  }
  for (; (*p != '\0') && (i < 64); p++) {
    char c = *p;
    if ((c == '/') || (c == ':') || (c == '?') || (c == '#')) {
      if (partition == NULL || partition[0] == '\0') {
        break; /* end of the host name */
      }
    }
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
        ((c >= '0') && (c <= '9')) || (c == '-') || (c == '_') ||
        (c == '.')) {
      tmp[i++] = c;
    } else {
      tmp[i++] = '_';
    }
  }
  /* WebView2 refuses a name ending in '.' or ' ', and "default" (in any
     case) IS the host's own profile - either would quietly put the guest in
     the host's storage, so neither is allowed through */
  while ((i > 0) && ((tmp[i - 1] == '.') || (tmp[i - 1] == ' '))) {
    i--;
  }
  tmp[i] = '\0';
  if (i == 0) {
    strcpy(tmp, "guest");
  } else if (_stricmp(tmp, "default") == 0) {
    strcpy(tmp, "guest-default");
  }
  MultiByteToWideChar(CP_UTF8, 0, tmp, -1, out, 65);
}

/* --------------------------------------------------------------------------
 * The frame: a plain child window the guest lives in. Sized to the VISIBLE
 * part of the element, so a guest half scrolled out of a container is cut
 * off by the frame exactly where the container cuts off the element.
 * ------------------------------------------------------------------------*/
static LRESULT CALLBACK wvg_frame_proc(HWND hwnd, UINT msg, WPARAM wp,
                                       LPARAM lp) {
  if (msg == WM_ERASEBKGND) {
    return 1; /* the guest paints everything; no white flash */
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

static HWND wvg_create_frame(HWND parent) {
  static int registered = 0;
  HWND frame;
  if (!registered) {
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wvg_frame_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = WVG_FRAME_CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);
    registered = 1;
  }
  frame = CreateWindowExW(0, WVG_FRAME_CLASS, L"",
                          WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0,
                          0, parent, NULL, GetModuleHandleW(NULL), NULL);
  if (frame != NULL) {
    /* above the host page's own browser window */
    SetWindowPos(frame, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }
  return frame;
}

/* Cut the holes out of a window. `dx`,`dy` is where the window sits
   relative to the frame, since region coordinates are the window's own. The
   system owns the region once SetWindowRgn succeeds. */
static void wvg_shape_window(HWND hwnd, struct webview_guest *g, int dx,
                             int dy, int width, int height) {
  HRGN rgn;
  int i;
  if (g->hole_count == 0) {
    SetWindowRgn(hwnd, NULL, TRUE);
    return;
  }
  rgn = CreateRectRgn(0, 0, width, height);
  if (rgn == NULL) {
    return;
  }
  for (i = 0; i < g->hole_count; i++) {
    int x = g->holes[i][0] - dx;
    int y = g->holes[i][1] - dy;
    int w = g->holes[i][2];
    int h = g->holes[i][3];
    int r = g->holes[i][4];
    /* +1: region rectangles exclude their right and bottom edges */
    HRGN hole = (r > 1) ? CreateRoundRectRgn(x, y, x + w + 1, y + h + 1,
                                             r * 2, r * 2)
                        : CreateRectRgn(x, y, x + w, y + h);
    if (hole != NULL) {
      CombineRgn(rgn, rgn, hole, RGN_DIFF);
      DeleteObject(hole);
    }
  }
  if (!SetWindowRgn(hwnd, rgn, TRUE)) {
    DeleteObject(rgn);
  }
}

/* The frame gets the region, and so does every window WebView2 put inside
   it: a parent's region does clip its children's painting, but the page is
   presented through DirectComposition on WebView2's own child window, and
   shaping that window too makes sure both the picture and mouse input
   follow the holes - clicks in a hole reach your page, not the site. */
static void wvg_apply_shape(struct webview_guest *g) {
  HWND child;
  if ((g->hole_count == 0) && !g->shaped) {
    return;
  }
  wvg_shape_window(g->frame, g, 0, 0, g->cw, g->ch);
  for (child = GetWindow(g->frame, GW_CHILD); child != NULL;
       child = GetWindow(child, GW_HWNDNEXT)) {
    RECT r;
    POINT pt;
    GetWindowRect(child, &r);
    pt.x = r.left;
    pt.y = r.top;
    ScreenToClient(g->frame, &pt);
    wvg_shape_window(child, g, pt.x, pt.y, r.right - r.left, r.bottom - r.top);
  }
  g->shaped = (g->hole_count > 0);
}

/* Take every hole out again: in fullscreen nothing of the host is on top. */
static void wvg_clear_shape(struct webview_guest *g) {
  HWND child;
  if (!g->shaped) {
    return;
  }
  SetWindowRgn(g->frame, NULL, TRUE);
  for (child = GetWindow(g->frame, GW_CHILD); child != NULL;
       child = GetWindow(child, GW_HWNDNEXT)) {
    SetWindowRgn(child, NULL, TRUE);
  }
  g->shaped = 0;
}

/* Put the frame and the guest where the host says the element is - or, while
   the guest page is in fullscreen, over the whole window. The host keeps
   sending the element's box meanwhile, so leaving fullscreen puts the guest
   straight back where the element is now. */
static void wvg_apply_bounds(struct webview_guest *g) {
  int show;
  if (g->closed || (g->frame == NULL)) {
    return;
  }
  if (g->fullscreen && (g->owner != NULL)) {
    RECT client;
    GetClientRect(g->owner->priv.hwnd, &client);
    wvg_clear_shape(g);
    /* HWND_TOP: above the host page and above any other guest */
    SetWindowPos(g->frame, HWND_TOP, 0, 0, client.right, client.bottom,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    if (g->controller != NULL) {
      g->controller->lpVtbl->put_Bounds(g->controller, client);
      g->controller->lpVtbl->put_IsVisible(g->controller, TRUE);
    }
    return;
  }
  show = g->visible && !g->failed && !g->frozen && g->loaded_once &&
         (g->cw > 0) && (g->ch > 0);
  if (show) {
    SetWindowPos(g->frame, NULL, g->cx, g->cy, g->cw, g->ch,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  } else {
    ShowWindow(g->frame, SW_HIDE);
  }
  if (g->controller != NULL) {
    RECT r;
    /* relative to the frame, so the part outside the frame is clipped */
    r.left = g->x - g->cx;
    r.top = g->y - g->cy;
    r.right = r.left + (g->w > 0 ? g->w : 0);
    r.bottom = r.top + (g->h > 0 ? g->h : 0);
    g->controller->lpVtbl->put_Bounds(g->controller, r);
    /* Only "not rendered at all" counts as invisible to the page. Scrolled
       out of view keeps running (a video keeps playing), like a real tab
       that is merely scrolled away. */
    g->controller->lpVtbl->put_IsVisible(
        g->controller, (g->visible && !g->failed) ? TRUE : FALSE);
  }
  if (show) {
    wvg_apply_shape(g);
  }
}

/* n integers with commas between them, as sscanf("%d,%d,...") reads them:
   how many there were, and *end just after the last. Not sscanf itself:
   MinGW's own scanf comes with it, and that needs _strtoi64, which the
   msvcrt.dll of Windows 2000 does not have - the exe would not start. */
static int wvg_read_ints(const char *p, int *v, int n, const char **end) {
  int i;
  for (i = 0; i < n; i++) {
    char *after;
    long x;
    if (i > 0) {
      if (*p != ',') {
        break;
      }
      p++;
    }
    x = strtol(p, &after, 10);
    if (after == p) {
      break;
    }
    v[i] = (int)x;
    p = after;
  }
  if (end != NULL) {
    *end = p;
  }
  return i;
}

/* "x,y,w,h,r;x,y,w,h,r;..." after the ';' that ends the bounds */
static void wvg_parse_holes(struct webview_guest *g, const char *p) {
  g->hole_count = 0;
  while ((p != NULL) && (*p == ';')) {
    int v[5];
    p++;
    if (wvg_read_ints(p, v, 5, &p) != 5) {
      break;
    }
    if ((v[2] <= 0) || (v[3] <= 0)) {
      continue;
    }
    if (g->hole_count == g->hole_capacity) {
      int cap = g->hole_capacity ? g->hole_capacity * 2 : 8;
      int(*grown)[5];
      if (cap > 256) {
        break; /* nobody needs more; a runaway page should not cost more */
      }
      grown = (int(*)[5])realloc(g->holes, sizeof(int[5]) * (size_t)cap);
      if (grown == NULL) {
        break;
      }
      g->holes = grown;
      g->hole_capacity = cap;
    }
    memcpy(g->holes[g->hole_count++], v, sizeof(v));
  }
}

/* --------------------------------------------------------------------------
 * Guest event handlers
 * ------------------------------------------------------------------------*/

static HRESULT STDMETHODCALLTYPE wvg_on_nav_starting(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2NavigationStartingEventArgs *args) {
  struct webview_guest *g = This->g;
  LPWSTR uri = NULL;
  BOOL redirect = FALSE;
  BOOL user = FALSE;
  char *juri;
  char detail[256];
  char *msg;
  size_t n;
  if (g->closed) {
    return S_OK;
  }
  args->lpVtbl->get_Uri(args, &uri);
  args->lpVtbl->get_IsRedirected(args, &redirect);
  args->lpVtbl->get_IsUserInitiated(args, &user);
  juri = wvg_take_wstr_json(uri);
  if (juri == NULL) {
    return S_OK;
  }
  n = strlen(juri) + sizeof(detail);
  msg = (char *)malloc(n);
  if (msg != NULL) {
    snprintf(msg, n, "{\"url\":%s,\"isRedirect\":%s,\"isUserInitiated\":%s}",
             juri, redirect ? "true" : "false", user ? "true" : "false");
    wvg_emit(g, "did-start-navigation", msg);
    free(msg);
  }
  free(juri);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_nav_completed(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2NavigationCompletedEventArgs *args) {
  struct webview_guest *g = This->g;
  BOOL ok = FALSE;
  COREWEBVIEW2_WEB_ERROR_STATUS status = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
  int http = 0;
  ICoreWebView2NavigationCompletedEventArgs2 *args2 = NULL;
  LPWSTR src = NULL;
  char *jsrc;
  char *msg;
  size_t n;
  int show_fallback;
  if (g->closed) {
    return S_OK;
  }
  args->lpVtbl->get_IsSuccess(args, &ok);
  args->lpVtbl->get_WebErrorStatus(args, &status);
  if (SUCCEEDED(args->lpVtbl->QueryInterface(
          args, iid_unref(&IID_ICoreWebView2NavigationCompletedEventArgs2),
          (void **)&args2)) &&
      (args2 != NULL)) {
    args2->lpVtbl->get_HttpStatusCode(args2, &http);
    args2->lpVtbl->Release(args2);
  }
  sender->lpVtbl->get_Source(sender, &src);
  jsrc = wvg_take_wstr_json(src);
  if (jsrc == NULL) {
    return S_OK;
  }
  /* A navigation replaced by another one is not a failure. An HTTP error
     (404, 500...) came with a real page from the server, so show that page;
     the fallback is for "there was no page at all" - offline, DNS, TLS... */
  show_fallback = !ok && (status != COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED) &&
                  (http == 0);
  if (ok || !show_fallback) {
    if (status != COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED) {
      g->failed = 0;
      g->loaded_once = 1;
    }
  } else {
    g->failed = 1;
  }
  wvg_apply_bounds(g);
  n = strlen(jsrc) + 160;
  msg = (char *)malloc(n);
  if (msg != NULL) {
    if (ok) {
      snprintf(msg, n, "{\"url\":%s,\"httpStatusCode\":%d}", jsrc, http);
      wvg_emit(g, "did-finish-load", msg);
    } else {
      snprintf(msg, n,
               "{\"url\":%s,\"errorCode\":%d,\"httpStatusCode\":%d,"
               "\"showFallback\":%s}",
               jsrc, (int)status, http, show_fallback ? "true" : "false");
      wvg_emit(g, "did-fail-load", msg);
    }
    free(msg);
  }
  free(jsrc);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_title(wvg_obj *This,
                                              ICoreWebView2 *sender,
                                              IUnknown *args) {
  LPWSTR title = NULL;
  char *jt;
  char *msg;
  size_t n;
  if (This->g->closed) {
    return S_OK;
  }
  sender->lpVtbl->get_DocumentTitle(sender, &title);
  jt = wvg_take_wstr_json(title);
  if (jt == NULL) {
    return S_OK;
  }
  n = strlen(jt) + 32;
  msg = (char *)malloc(n);
  if (msg != NULL) {
    snprintf(msg, n, "{\"title\":%s}", jt);
    wvg_emit(This->g, "page-title-updated", msg);
    free(msg);
  }
  free(jt);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_source(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2SourceChangedEventArgs *args) {
  LPWSTR src = NULL;
  BOOL isNew = TRUE;
  char *js;
  char *msg;
  size_t n;
  if (This->g->closed) {
    return S_OK;
  }
  args->lpVtbl->get_IsNewDocument(args, &isNew);
  sender->lpVtbl->get_Source(sender, &src);
  js = wvg_take_wstr_json(src);
  if (js == NULL) {
    return S_OK;
  }
  n = strlen(js) + 32;
  msg = (char *)malloc(n);
  if (msg != NULL) {
    snprintf(msg, n, "{\"url\":%s}", js);
    wvg_emit(This->g, isNew ? "did-navigate" : "did-navigate-in-page", msg);
    free(msg);
  }
  free(js);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_history(wvg_obj *This,
                                                ICoreWebView2 *sender,
                                                IUnknown *args) {
  BOOL back = FALSE;
  BOOL fwd = FALSE;
  char msg[80];
  if (This->g->closed) {
    return S_OK;
  }
  sender->lpVtbl->get_CanGoBack(sender, &back);
  sender->lpVtbl->get_CanGoForward(sender, &fwd);
  snprintf(msg, sizeof(msg), "{\"canGoBack\":%s,\"canGoForward\":%s}",
           back ? "true" : "false", fwd ? "true" : "false");
  wvg_emit(This->g, "history-changed", msg);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_new_window(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2NewWindowRequestedEventArgs *args) {
  struct webview_guest *g = This->g;
  LPWSTR uri = NULL;
  char *ju;
  char *msg;
  size_t n;
  if (g->closed) {
    return S_OK;
  }
  args->lpVtbl->get_Uri(args, &uri);
  if (!g->popups && (uri != NULL)) {
    /* default: target=_blank and window.open() load in this same guest,
       the way a single-tab browser would */
    args->lpVtbl->put_Handled(args, TRUE);
    sender->lpVtbl->Navigate(sender, uri);
  }
  ju = wvg_take_wstr_json(uri);
  if (ju == NULL) {
    return S_OK;
  }
  n = strlen(ju) + 64;
  msg = (char *)malloc(n);
  if (msg != NULL) {
    snprintf(msg, n, "{\"url\":%s,\"popup\":%s}", ju,
             g->popups ? "true" : "false");
    wvg_emit(g, "new-window", msg);
    free(msg);
  }
  free(ju);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_process_failed(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2ProcessFailedEventArgs *args) {
  COREWEBVIEW2_PROCESS_FAILED_KIND kind =
      COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED;
  char msg[64];
  if (This->g->closed) {
    return S_OK;
  }
  args->lpVtbl->get_ProcessFailedKind(args, &kind);
  snprintf(msg, sizeof(msg), "{\"kind\":%d}", (int)kind);
  wvg_emit(This->g, "render-process-gone", msg);
  return S_OK;
}

/* The guest page called element.requestFullscreen() - a video player's
   fullscreen button, usually. Left alone, the video would only fill the
   <webview> element's box. A browser fills the screen, so: the guest covers
   the whole window and the window goes fullscreen (wv_options.inl), and
   both come back when the page leaves fullscreen. The element hears about
   it as 'enter-html-full-screen' / 'leave-html-full-screen', the same
   names Electron's <webview> uses. */
static HRESULT STDMETHODCALLTYPE wvg_on_fullscreen(wvg_obj *This,
                                                   ICoreWebView2 *sender,
                                                   IUnknown *args) {
  struct webview_guest *g = This->g;
  BOOL contains = FALSE;
  int follow;
  if (g->closed) {
    return S_OK;
  }
  sender->lpVtbl->get_ContainsFullScreenElement(sender, &contains);
  follow = contains && webview_html_fullscreen_allowed(g->owner);
  if (follow != g->fullscreen) {
    g->fullscreen = follow;
    wvg_apply_bounds(g);
    webview_html_fullscreen_changed(g->owner);
    /* the window may already have been fullscreen, so no resize came */
    wvg_apply_bounds(g);
  }
  wvg_emit(g, contains ? "enter-html-full-screen" : "leave-html-full-screen",
           "{}");
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE wvg_on_accelerator(
    wvg_obj *This, ICoreWebView2Controller *sender,
    ICoreWebView2AcceleratorKeyPressedEventArgs *args) {
  if (!This->g->closed) {
    WebView2EscapeFromFullscreen(sender, This->g->owner->priv.hwnd, args);
  }
  return S_OK;
}

/* WM_SIZE: a fullscreen guest follows the window. */
static void webview_guests_parent_resized(struct webview *w) {
  struct webview_guest *g;
  for (g = (w != NULL) ? w->priv.guests : NULL; g != NULL; g = g->next) {
    if (g->fullscreen) {
      wvg_apply_bounds(g);
    }
  }
}

static int webview_guests_any_fullscreen(struct webview *w) {
  struct webview_guest *g;
  for (g = (w != NULL) ? w->priv.guests : NULL; g != NULL; g = g->next) {
    if (g->fullscreen && !g->closed) {
      return 1;
    }
  }
  return 0;
}

/* webview_window_set_fullscreen(w, 0): take the guests out too. */
static void webview_guests_exit_fullscreen(struct webview *w) {
  struct webview_guest *g;
  for (g = (w != NULL) ? w->priv.guests : NULL; g != NULL; g = g->next) {
    if (g->fullscreen && (g->view != NULL)) {
      WebView2ExecuteOn(g->view, w->priv.hwnd, WEBVIEW_JS_EXIT_FULLSCREEN,
                        NULL, NULL);
    }
  }
}

/* --------------------------------------------------------------------------
 * Dragging content out of a guest
 *
 * When the guest's picker starts a drag, the mouse button is down inside the
 * guest's window, and the cursor then travels over the host page - a
 * different window in a different process. Rather than depend on who holds
 * mouse capture, C simply follows the cursor with a timer until the button
 * comes up (drop) or Escape is pressed (cancel), and tells the host page
 * where it is. One drag at a time, application wide.
 * ------------------------------------------------------------------------*/
static struct {
  struct webview *w;
  int id;
  int active;
  POINT last;
} wvg_drag;

static void wvg_drag_stop(const char *phase) {
  char msg[160];
  POINT pt;
  struct webview *w = wvg_drag.w;
  struct webview_guest *g;
  if (!wvg_drag.active || (w == NULL)) {
    return;
  }
  wvg_drag.active = 0;
  KillTimer(w->priv.hwnd, WEBVIEW_GUEST_DRAG_TIMER);
  GetCursorPos(&pt);
  ScreenToClient(w->priv.hwnd, &pt);
  snprintf(msg, sizeof(msg),
           "{\"__wvg\":\"drag\",\"id\":%d,\"phase\":\"%s\",\"x\":%ld,\"y\":%ld}",
           wvg_drag.id, phase, (long)pt.x, (long)pt.y);
  wvg_host_post(w, msg);
  /* let the picker in the guest reset its own state */
  g = wvg_find(w, wvg_drag.id);
  if ((g != NULL) && (g->view != NULL)) {
    WebView2ExecuteOn(g->view, w->priv.hwnd,
                      "window.__wvAgent&&window.__wvAgent.dragEnded();", NULL,
                      NULL);
  }
}

static void webview_guest_drag_tick(struct webview *w) {
  POINT pt;
  char msg[160];
  int button;
  if (!wvg_drag.active || (w != wvg_drag.w)) {
    KillTimer(w->priv.hwnd, WEBVIEW_GUEST_DRAG_TIMER);
    return;
  }
  if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
    wvg_drag_stop("cancel");
    return;
  }
  button = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
  if (!(GetAsyncKeyState(button) & 0x8000)) {
    wvg_drag_stop("drop");
    return;
  }
  GetCursorPos(&pt);
  ScreenToClient(w->priv.hwnd, &pt);
  if ((pt.x == wvg_drag.last.x) && (pt.y == wvg_drag.last.y)) {
    return;
  }
  wvg_drag.last = pt;
  snprintf(msg, sizeof(msg),
           "{\"__wvg\":\"drag\",\"id\":%d,\"phase\":\"move\",\"x\":%ld,\"y\":%ld}",
           wvg_drag.id, (long)pt.x, (long)pt.y);
  wvg_host_post(w, msg);
}

static void wvg_drag_start(struct webview_guest *g) {
  if (wvg_drag.active) {
    wvg_drag_stop("cancel");
  }
  wvg_drag.w = g->owner;
  wvg_drag.id = g->id;
  wvg_drag.active = 1;
  wvg_drag.last.x = wvg_drag.last.y = -100000;
  SetTimer(g->owner->priv.hwnd, WEBVIEW_GUEST_DRAG_TIMER, 15, NULL);
}

/* Messages from inside the guest. The agent's own ones start with
   "__wvga\x1f<type>\x1f"; everything else is the page talking and becomes an
   'ipc-message' event. The page CAN imitate the agent - it is the same
   JavaScript world - so the host treats every payload as untrusted data. */
static HRESULT STDMETHODCALLTYPE wvg_on_message(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2WebMessageReceivedEventArgs *args) {
  struct webview_guest *g = This->g;
  LPWSTR ws = NULL;
  char *text = NULL;
  char *msg;
  size_t n;
  if (g->closed) {
    return S_OK;
  }
  if (SUCCEEDED(args->lpVtbl->TryGetWebMessageAsString(args, &ws)) &&
      (ws != NULL)) {
    text = webview_from_utf16(ws);
    CoTaskMemFree(ws);
    ws = NULL;
  }
  if ((text != NULL) &&
      (strncmp(text, WVG_AGENT_PREFIX, sizeof(WVG_AGENT_PREFIX) - 1) == 0)) {
    char *type = text + sizeof(WVG_AGENT_PREFIX) - 1;
    char *data = strchr(type, '\x1f');
    char *jtype;
    char *jdata;
    if (data != NULL) {
      *data++ = '\0';
    } else {
      data = (char *)"null";
    }
    if (strcmp(type, "drag-start") == 0) {
      wvg_drag_start(g);
    }
    /* forwarded as a JSON string; the host parses it defensively */
    jtype = webview_json_escape(type);
    jdata = webview_json_escape(data);
    if ((jtype != NULL) && (jdata != NULL)) {
      n = strlen(jtype) + strlen(jdata) + 80;
      msg = (char *)malloc(n);
      if (msg != NULL) {
        snprintf(msg, n,
                 "{\"__wvg\":\"agent\",\"id\":%d,\"type\":%s,\"data\":%s}",
                 g->id, jtype, jdata);
        wvg_host_post(g->owner, msg);
        free(msg);
      }
    }
    free(jtype);
    free(jdata);
    GlobalFree(text);
    return S_OK;
  }
  if (text != NULL) {
    GlobalFree(text);
  }
  /* anything else: pass it on as JSON, exactly as the page sent it */
  if (SUCCEEDED(args->lpVtbl->get_WebMessageAsJson(args, &ws)) &&
      (ws != NULL)) {
    char *json = webview_from_utf16(ws);
    CoTaskMemFree(ws);
    if (json != NULL) {
      n = strlen(json) + 32;
      msg = (char *)malloc(n);
      if (msg != NULL) {
        snprintf(msg, n, "{\"data\":%s}", json);
        wvg_emit(g, "ipc-message", msg);
        free(msg);
      }
      GlobalFree(json);
    }
  }
  return S_OK;
}

/* A handler for AddScriptToExecuteOnDocumentCreated that does nothing. */
static HRESULT STDMETHODCALLTYPE wvg_on_script_added(wvg_obj *This,
                                                     HRESULT hr, LPCWSTR id) {
  if (FAILED(hr)) {
    webview_print_log("guest: AddScriptToExecuteOnDocumentCreated failed");
  }
  /* a guest closed in the meantime belonged to a page that may be gone,
     and its request numbers mean nothing to the page there now */
  if ((This->g != NULL) && !This->g->closed) {
    wvg_settle(This->g->owner, This->req, SUCCEEDED(hr), "null");
  }
  return S_OK;
}

WVG_VTBL(wvg_vtbl_nav_starting, wvg_on_nav_starting,
         &IID_ICoreWebView2NavigationStartingEventHandler);
WVG_VTBL(wvg_vtbl_nav_completed, wvg_on_nav_completed,
         &IID_ICoreWebView2NavigationCompletedEventHandler);
WVG_VTBL(wvg_vtbl_title, wvg_on_title,
         &IID_ICoreWebView2DocumentTitleChangedEventHandler);
WVG_VTBL(wvg_vtbl_source, wvg_on_source,
         &IID_ICoreWebView2SourceChangedEventHandler);
WVG_VTBL(wvg_vtbl_history, wvg_on_history,
         &IID_ICoreWebView2HistoryChangedEventHandler);
WVG_VTBL(wvg_vtbl_new_window, wvg_on_new_window,
         &IID_ICoreWebView2NewWindowRequestedEventHandler);
WVG_VTBL(wvg_vtbl_message, wvg_on_message,
         &IID_ICoreWebView2WebMessageReceivedEventHandler);
WVG_VTBL(wvg_vtbl_process_failed, wvg_on_process_failed,
         &IID_ICoreWebView2ProcessFailedEventHandler);
WVG_VTBL(wvg_vtbl_script_added, wvg_on_script_added,
         &IID_ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler);
WVG_VTBL(wvg_vtbl_fullscreen, wvg_on_fullscreen,
         &IID_ICoreWebView2ContainsFullScreenElementChangedEventHandler);
WVG_VTBL(wvg_vtbl_accelerator, wvg_on_accelerator,
         &IID_ICoreWebView2AcceleratorKeyPressedEventHandler);

/* Register one event handler; `add` is the add_Xxx method. */
#define WVG_ADD(g, slot, method, vtbl, type)                                   \
  do {                                                                         \
    wvg_obj *o_ = wvg_obj_new(&(vtbl), (g), 0);                                \
    if (o_ != NULL) {                                                          \
      if (SUCCEEDED((g)->view->lpVtbl->method((g)->view, (type *)o_,           \
                                              &(g)->tok[slot]))) {             \
        (g)->has_tok[slot] = 1;                                                \
      }                                                                        \
      o_->lpVtbl->Release(o_);                                                 \
    }                                                                          \
  } while (0)

static int wvg_add_script(struct webview_guest *g, const char *js, int req) {
  WCHAR *wjs;
  wvg_obj *o;
  HRESULT hr;
  if ((g->view == NULL) || (js == NULL)) {
    return -1;
  }
  wjs = webview_to_utf16(js);
  o = wvg_obj_new(&wvg_vtbl_script_added, g, req);
  if ((wjs == NULL) || (o == NULL)) {
    if (wjs != NULL) {
      GlobalFree(wjs);
    }
    if (o != NULL) {
      o->lpVtbl->Release(o);
    }
    return -1;
  }
  hr = g->view->lpVtbl->AddScriptToExecuteOnDocumentCreated(
      g->view, wjs,
      (ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler *)o);
  o->lpVtbl->Release(o);
  GlobalFree(wjs);
  return SUCCEEDED(hr) ? 0 : -1;
}

/* --------------------------------------------------------------------------
 * Look: opacity and transparency
 *
 * Host CSS cannot reach the guest surface, so js/guest_host.js works out
 * what it should look like and sends it here:
 *
 *   opacity    the element's opacity times every ancestor's (0..1000)
 *   mode       0  opaque (the default)
 *              1  transparent: where the guest page paints nothing, the
 *                 host page shows through - like an iframe
 *              2  transparent="force": the guest page's own background is
 *                 taken away as well
 *
 * Two halves. Here: the guest's default background (the sheet WebView2
 * paints under every page) is transparent whenever anything is asked for.
 * In the guest: __wvAgent.look() fades the page and repaints the parts of
 * its background that need it - see js/guest_agent.js. The agent is told
 * twice: once now for the page that is there, and by a document-created
 * script for every page after it, so a navigation never shows a flash of
 * the plain look.
 *
 * The transparent guest is composited over the host page by Windows
 * (DirectComposition): no snapshots, no copies, live at full frame rate.
 * ------------------------------------------------------------------------*/
static int wvg_look_plain(const struct webview_guest *g) {
  return (g->look_opacity >= 1000) && (g->look_mode == 0);
}

static void wvg_look_js(const struct webview_guest *g, char *out, size_t n) {
  snprintf(out, n, "window.__wvAgent&&__wvAgent.look&&__wvAgent.look(%d/1000,%d)",
           g->look_opacity, g->look_mode);
}

static void wvg_refresh_look_script(struct webview_guest *g);

static HRESULT STDMETHODCALLTYPE wvg_on_look_script_added(wvg_obj *This,
                                                          HRESULT hr,
                                                          LPCWSTR id) {
  struct webview_guest *g = This->g;
  if (g == NULL) {
    return S_OK;
  }
  g->look_busy = 0;
  if (SUCCEEDED(hr) && (id != NULL)) {
    size_t n = wcslen(id) + 1;
    free(g->look_script);
    g->look_script = (WCHAR *)malloc(n * sizeof(WCHAR));
    if (g->look_script != NULL) {
      memcpy(g->look_script, id, n * sizeof(WCHAR));
    }
  } else {
    webview_print_log("guest: could not install the look script");
  }
  if (g->closed) {
    return S_OK;
  }
  if (g->look_dirty) {
    g->look_dirty = 0;
    wvg_refresh_look_script(g);
  }
  return S_OK;
}

WVG_VTBL(wvg_vtbl_look_script_added, wvg_on_look_script_added,
         &IID_ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler);

/* One document-created script at a time carries the look: the old one is
   removed, the new one added. Changes that arrive while an add is still in
   flight (a slider being dragged) are folded into one more add after it. */
static void wvg_refresh_look_script(struct webview_guest *g) {
  char js[128];
  WCHAR *wjs;
  wvg_obj *o;
  if ((g->view == NULL) || g->closed) {
    return;
  }
  if (g->look_busy) {
    g->look_dirty = 1;
    return;
  }
  if (g->look_script != NULL) {
    g->view->lpVtbl->RemoveScriptToExecuteOnDocumentCreated(g->view,
                                                            g->look_script);
    free(g->look_script);
    g->look_script = NULL;
  }
  if (wvg_look_plain(g)) {
    return;
  }
  wvg_look_js(g, js, sizeof(js));
  wjs = webview_to_utf16(js);
  o = wvg_obj_new(&wvg_vtbl_look_script_added, g, 0);
  if ((wjs != NULL) && (o != NULL)) {
    g->look_busy = 1;
    if (FAILED(g->view->lpVtbl->AddScriptToExecuteOnDocumentCreated(
            g->view, wjs,
            (ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler *)
                o))) {
      g->look_busy = 0;
    }
  }
  if (o != NULL) {
    o->lpVtbl->Release(o);
  }
  if (wjs != NULL) {
    GlobalFree(wjs);
  }
}

WEBVIEW_API int webview_runtime_transparency_broken(void); /* wv_transparent */

/* The guest's default background: transparent, or the browser's white. */
static void wvg_apply_look_background(struct webview_guest *g) {
  ICoreWebView2Controller2 *c2 = NULL;
  COREWEBVIEW2_COLOR color;
  int clear = !wvg_look_plain(g);
  if ((g->controller == NULL) || (clear == g->look_clear)) {
    return;
  }
  if (FAILED(g->controller->lpVtbl->QueryInterface(
          g->controller, iid_unref(&IID_ICoreWebView2Controller2),
          (void **)&c2)) ||
      (c2 == NULL)) {
    webview_print_log("guest: transparency needs WebView2 runtime "
                      "1.0.774.44 or newer");
    g->look_clear = clear; /* do not retry on every change */
    return;
  }
  color.A = clear ? 0 : 255;
  color.R = 255;
  color.G = 255;
  color.B = 255;
  if (SUCCEEDED(c2->lpVtbl->put_DefaultBackgroundColor(c2, color))) {
    g->look_clear = clear;
  }
  c2->lpVtbl->Release(c2);
  if (clear && webview_runtime_transparency_broken()) {
    webview_print_log("guest: this WebView2 runtime (144/145) paints "
                      "transparent backgrounds solid - see TRANSPARENCY.md");
  }
}

/* After the host changed the look. Before the controller exists the values
   are only kept: wvg_on_controller applies them before the first page. */
static void wvg_apply_look(struct webview_guest *g, int opacity, int mode) {
  char js[128];
  int was_plain = wvg_look_plain(g);
  if (opacity < 0) opacity = 0;
  if (opacity > 1000) opacity = 1000;
  if ((mode < 0) || (mode > 2)) mode = 0;
  if ((opacity == g->look_opacity) && (mode == g->look_mode)) {
    return;
  }
  g->look_opacity = opacity;
  g->look_mode = mode;
  if (g->view == NULL) {
    return;
  }
  wvg_apply_look_background(g);
  /* the page that is there now (back to plain clears the agent's sheet) */
  if (!was_plain || !wvg_look_plain(g)) {
    wvg_look_js(g, js, sizeof(js));
    WebView2ExecuteOn(g->view, g->owner->priv.hwnd, js, NULL, NULL);
  }
  wvg_refresh_look_script(g);
}

/* The host's paint order for its guests, bottom first: "3,1,2". Every frame
   goes to the top in that order, so the last named ends up uppermost; a
   guest in fullscreen stays above them all. */
static void wvg_apply_order(struct webview *w, const char *list) {
  struct webview_guest *g;
  const char *p = list;
  while ((p != NULL) && (*p != '\0')) {
    char *end = NULL;
    long id = strtol(p, &end, 10);
    if (end == p) {
      break;
    }
    g = wvg_find(w, (int)id);
    if ((g != NULL) && (g->frame != NULL) && !g->fullscreen) {
      SetWindowPos(g->frame, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    p = (*end == ',') ? end + 1 : NULL;
  }
  for (g = w->priv.guests; g != NULL; g = g->next) {
    if (g->fullscreen && (g->frame != NULL) && !g->closed) {
      SetWindowPos(g->frame, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
  }
}

/* --------------------------------------------------------------------------
 * Creation
 * ------------------------------------------------------------------------*/

static void wvg_navigate(struct webview_guest *g, const char *url) {
  WCHAR *wurl;
  HRESULT hr;
  if ((g->view == NULL) || (url == NULL) || (url[0] == '\0')) {
    return;
  }
  wurl = webview_to_utf16(url);
  if (wurl == NULL) {
    return;
  }
  hr = g->view->lpVtbl->Navigate(g->view, wurl);
  GlobalFree(wurl);
  if (FAILED(hr)) {
    char detail[96];
    snprintf(detail, sizeof(detail),
             "{\"url\":\"\",\"errorCode\":-1,\"httpStatusCode\":0,"
             "\"showFallback\":true}");
    g->failed = 1;
    wvg_apply_bounds(g);
    wvg_emit(g, "did-fail-load", detail);
  }
}

static void wvg_unsupported(struct webview_guest *g, const char *why) {
  char *jw = webview_json_escape(why);
  char *msg;
  size_t n;
  g->failed = 1;
  if (jw == NULL) {
    return;
  }
  n = strlen(jw) + 32;
  msg = (char *)malloc(n);
  if (msg != NULL) {
    snprintf(msg, n, "{\"reason\":%s}", jw);
    wvg_emit(g, "unsupported", msg);
    free(msg);
  }
  free(jw);
  webview_print_log(why);
}

static HRESULT STDMETHODCALLTYPE wvg_on_controller(
    wvg_obj *This, HRESULT result, ICoreWebView2Controller *controller) {
  struct webview_guest *g = This->g;
  struct webview *w = g->owner;
  ICoreWebView2Settings *settings = NULL;
  BOOL devtools;
  int i;
  if (FAILED(result) || (controller == NULL)) {
    if (!g->closed) {
      wvg_unsupported(g, "guest: the browser could not create the page");
    }
    return S_OK;
  }
  if (g->closed) {
    /* the element went away while we were waiting */
    controller->lpVtbl->Close(controller);
    return S_OK;
  }
  controller->lpVtbl->AddRef(controller);
  g->controller = controller;
  controller->lpVtbl->get_CoreWebView2(controller, &g->view);
  if (g->view == NULL) {
    wvg_unsupported(g, "guest: no page object");
    return S_OK;
  }

  if (SUCCEEDED(g->view->lpVtbl->get_Settings(g->view, &settings)) &&
      (settings != NULL)) {
    devtools = (w->browser.devtools == WEBVIEW_ON) ||
               ((w->browser.devtools == WEBVIEW_DEFAULT) && w->debug);
    settings->lpVtbl->put_AreDevToolsEnabled(settings, devtools);
    settings->lpVtbl->put_IsWebMessageEnabled(settings, TRUE);
    settings->lpVtbl->put_AreHostObjectsAllowed(settings, FALSE);
    settings->lpVtbl->Release(settings);
  }

  WVG_ADD(g, WVG_TOK_NAV_STARTING, add_NavigationStarting,
          wvg_vtbl_nav_starting, ICoreWebView2NavigationStartingEventHandler);
  WVG_ADD(g, WVG_TOK_NAV_COMPLETED, add_NavigationCompleted,
          wvg_vtbl_nav_completed, ICoreWebView2NavigationCompletedEventHandler);
  WVG_ADD(g, WVG_TOK_TITLE, add_DocumentTitleChanged, wvg_vtbl_title,
          ICoreWebView2DocumentTitleChangedEventHandler);
  WVG_ADD(g, WVG_TOK_SOURCE, add_SourceChanged, wvg_vtbl_source,
          ICoreWebView2SourceChangedEventHandler);
  WVG_ADD(g, WVG_TOK_HISTORY, add_HistoryChanged, wvg_vtbl_history,
          ICoreWebView2HistoryChangedEventHandler);
  WVG_ADD(g, WVG_TOK_NEW_WINDOW, add_NewWindowRequested, wvg_vtbl_new_window,
          ICoreWebView2NewWindowRequestedEventHandler);
  WVG_ADD(g, WVG_TOK_MESSAGE, add_WebMessageReceived, wvg_vtbl_message,
          ICoreWebView2WebMessageReceivedEventHandler);
  WVG_ADD(g, WVG_TOK_PROCESS_FAILED, add_ProcessFailed,
          wvg_vtbl_process_failed, ICoreWebView2ProcessFailedEventHandler);
  WVG_ADD(g, WVG_TOK_FULLSCREEN, add_ContainsFullScreenElementChanged,
          wvg_vtbl_fullscreen,
          ICoreWebView2ContainsFullScreenElementChangedEventHandler);
  {
    /* Esc lives on the controller, not on the page */
    wvg_obj *o = wvg_obj_new(&wvg_vtbl_accelerator, g, 0);
    if (o != NULL) {
      if (SUCCEEDED(controller->lpVtbl->add_AcceleratorKeyPressed(
              controller, (ICoreWebView2AcceleratorKeyPressedEventHandler *)o,
              &g->accel_tok))) {
        g->has_accel_tok = 1;
      }
      o->lpVtbl->Release(o);
    }
  }

  /* the in-page helper first, then whatever the host queued up */
  wvg_add_script(g, WEBVIEW_JS_GUEST_AGENT, 0);
  for (i = 0; i < g->script_count; i++) {
    wvg_add_script(g, g->scripts[i], 0);
    free(g->scripts[i]);
  }
  free(g->scripts);
  g->scripts = NULL;
  g->script_count = 0;
  /* a look the host asked for before the page existed */
  if (!wvg_look_plain(g)) {
    wvg_apply_look_background(g);
    wvg_refresh_look_script(g);
  }

  wvg_apply_bounds(g);
  wvg_emit(g, "created", "{}");
  wvg_navigate(g, g->src);
  return S_OK;
}

WVG_VTBL(wvg_vtbl_controller, wvg_on_controller,
         &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler);

static void wvg_create(struct webview *w, int id, const char *src,
                       const char *partition, const char *flags) {
  struct webview_guest *g;
  webview2 *host = w->priv.webview2;
  ICoreWebView2Environment10 *env10 = NULL;
  ICoreWebView2ControllerOptions *opts = NULL;
  wvg_obj *handler;
  HRESULT hr;
  if (wvg_find(w, id) != NULL) {
    return; /* already there */
  }
  g = (struct webview_guest *)calloc(1, sizeof(struct webview_guest));
  if (g == NULL) {
    return;
  }
  g->ref = 1; /* the owner's list */
  g->owner = w;
  g->id = id;
  g->look_opacity = 1000;
  g->look_clear = -1; /* not applied yet: the runtime's own default */
  g->src = webview_strdup(src != NULL ? src : "");
  g->incognito = (flags != NULL) && (strchr(flags, 'i') != NULL);
  g->popups = (flags != NULL) && (strchr(flags, 'p') != NULL);
  wvg_profile_name(g->profile, partition, src);
  g->next = w->priv.guests;
  w->priv.guests = g;

  if ((host == NULL) || (host->env == NULL)) {
    wvg_unsupported(g, "guest: the host browser is not ready");
    return;
  }
  g->frame = wvg_create_frame(w->priv.hwnd);
  if (g->frame == NULL) {
    wvg_unsupported(g, "guest: could not create the child window");
    return;
  }
  handler = wvg_obj_new(&wvg_vtbl_controller, g, 0);
  if (handler == NULL) {
    return;
  }
  /* Separate profiles need ICoreWebView2Environment10 (runtime 101+, 2022).
     On anything older the guest still works, it just shares the host's
     cookies and storage - said out loud rather than silently. */
  if (SUCCEEDED(host->env->lpVtbl->QueryInterface(
          host->env, iid_unref(&IID_ICoreWebView2Environment10),
          (void **)&env10)) &&
      (env10 != NULL) &&
      SUCCEEDED(env10->lpVtbl->CreateCoreWebView2ControllerOptions(env10,
                                                                   &opts)) &&
      (opts != NULL)) {
    hr = opts->lpVtbl->put_ProfileName(opts, g->profile);
    if (SUCCEEDED(hr)) {
      opts->lpVtbl->put_IsInPrivateModeEnabled(opts,
                                               g->incognito ? TRUE : FALSE);
      hr = env10->lpVtbl->CreateCoreWebView2ControllerWithOptions(
          env10, g->frame, opts,
          (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *)handler);
    } else {
      /* never fall back to the shared profile over a bad name */
      webview_print_log("guest: the partition name was refused");
    }
    opts->lpVtbl->Release(opts);
  } else {
    webview_print_log("guest: runtime too old for separate profiles - "
                      "this <webview> shares the host's storage");
    hr = host->env->lpVtbl->CreateCoreWebView2Controller(
        host->env, g->frame,
        (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *)handler);
  }
  if (env10 != NULL) {
    env10->lpVtbl->Release(env10);
  }
  handler->lpVtbl->Release(handler);
  if (FAILED(hr)) {
    wvg_unsupported(g, "guest: CreateCoreWebView2Controller failed");
  }
}

/* --------------------------------------------------------------------------
 * Teardown
 * ------------------------------------------------------------------------*/

static void webview_guest_release(struct webview_guest *g) {
  int i;
  if (InterlockedDecrement(&g->ref) > 0) {
    return;
  }
  free(g->src);
  for (i = 0; i < g->script_count; i++) {
    free(g->scripts[i]);
  }
  free(g->scripts);
  free(g->holes);
  free(g->look_script);
  free(g);
}

static void wvg_close(struct webview_guest *g) {
  struct webview *w = g->owner;
  struct webview_guest **pp;
  int was_fullscreen;
  if (g->closed) {
    return;
  }
  g->closed = 1;
  was_fullscreen = g->fullscreen;
  g->fullscreen = 0;
  if (wvg_drag.active && (wvg_drag.w == w) && (wvg_drag.id == g->id)) {
    wvg_drag_stop("cancel");
  }
  for (pp = &w->priv.guests; *pp != NULL; pp = &(*pp)->next) {
    if (*pp == g) {
      *pp = g->next;
      break;
    }
  }
  if (g->view != NULL) {
    ICoreWebView2 *v = g->view;
    if (g->has_tok[WVG_TOK_NAV_STARTING])
      v->lpVtbl->remove_NavigationStarting(v, g->tok[WVG_TOK_NAV_STARTING]);
    if (g->has_tok[WVG_TOK_NAV_COMPLETED])
      v->lpVtbl->remove_NavigationCompleted(v, g->tok[WVG_TOK_NAV_COMPLETED]);
    if (g->has_tok[WVG_TOK_TITLE])
      v->lpVtbl->remove_DocumentTitleChanged(v, g->tok[WVG_TOK_TITLE]);
    if (g->has_tok[WVG_TOK_SOURCE])
      v->lpVtbl->remove_SourceChanged(v, g->tok[WVG_TOK_SOURCE]);
    if (g->has_tok[WVG_TOK_HISTORY])
      v->lpVtbl->remove_HistoryChanged(v, g->tok[WVG_TOK_HISTORY]);
    if (g->has_tok[WVG_TOK_NEW_WINDOW])
      v->lpVtbl->remove_NewWindowRequested(v, g->tok[WVG_TOK_NEW_WINDOW]);
    if (g->has_tok[WVG_TOK_MESSAGE])
      v->lpVtbl->remove_WebMessageReceived(v, g->tok[WVG_TOK_MESSAGE]);
    if (g->has_tok[WVG_TOK_PROCESS_FAILED])
      v->lpVtbl->remove_ProcessFailed(v, g->tok[WVG_TOK_PROCESS_FAILED]);
    if (g->has_tok[WVG_TOK_FULLSCREEN])
      v->lpVtbl->remove_ContainsFullScreenElementChanged(
          v, g->tok[WVG_TOK_FULLSCREEN]);
  }
  if ((g->controller != NULL) && g->has_accel_tok) {
    g->controller->lpVtbl->remove_AcceleratorKeyPressed(g->controller,
                                                        g->accel_tok);
    g->has_accel_tok = 0;
  }
  if (g->controller != NULL) {
    g->controller->lpVtbl->Close(g->controller);
    g->controller->lpVtbl->Release(g->controller);
    g->controller = NULL;
  }
  if (g->view != NULL) {
    g->view->lpVtbl->Release(g->view);
    g->view = NULL;
  }
  if (g->frame != NULL) {
    DestroyWindow(g->frame);
    g->frame = NULL;
  }
  webview_guest_release(g); /* the owner's reference */
  /* the element was removed while its page was fullscreen: the window
     comes back, as if the page had left fullscreen itself */
  if (was_fullscreen) {
    webview_html_fullscreen_changed(w);
  }
}

static void webview_guests_close_all(struct webview *w) {
  struct webview_guest *g;
  /* the window is going away - nothing to restore */
  for (g = (w != NULL) ? w->priv.guests : NULL; g != NULL; g = g->next) {
    g->fullscreen = 0;
  }
  while ((w != NULL) && (w->priv.guests != NULL)) {
    wvg_close(w->priv.guests);
  }
}

/* WM_MOVE: popups inside guests (a <select>, a date picker) are positioned
   in screen coordinates, so they need to hear that the window moved. */
static void webview_guests_parent_moved(struct webview *w) {
  struct webview_guest *g;
  for (g = (w != NULL) ? w->priv.guests : NULL; g != NULL; g = g->next) {
    if (g->controller != NULL) {
      g->controller->lpVtbl->NotifyParentWindowPositionChanged(g->controller);
    }
  }
}

/* --------------------------------------------------------------------------
 * capturePage(): a PNG of the guest as it looks right now
 * ------------------------------------------------------------------------*/

static char *wvg_base64(const unsigned char *p, size_t n) {
  static const char tbl[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t out_n = ((n + 2) / 3) * 4;
  char *out = (char *)malloc(out_n + 1);
  size_t i;
  size_t j = 0;
  if (out == NULL) {
    return NULL;
  }
  for (i = 0; i + 2 < n; i += 3) {
    unsigned long v = ((unsigned long)p[i] << 16) |
                      ((unsigned long)p[i + 1] << 8) | p[i + 2];
    out[j++] = tbl[(v >> 18) & 63];
    out[j++] = tbl[(v >> 12) & 63];
    out[j++] = tbl[(v >> 6) & 63];
    out[j++] = tbl[v & 63];
  }
  if (i < n) {
    unsigned long v = (unsigned long)p[i] << 16;
    if (i + 1 < n) {
      v |= (unsigned long)p[i + 1] << 8;
    }
    out[j++] = tbl[(v >> 18) & 63];
    out[j++] = tbl[(v >> 12) & 63];
    out[j++] = (i + 1 < n) ? tbl[(v >> 6) & 63] : '=';
    out[j++] = '=';
  }
  out[j] = '\0';
  return out;
}

static HRESULT STDMETHODCALLTYPE wvg_on_capture(wvg_obj *This, HRESULT hr) {
  struct webview *w = (This->g != NULL) ? This->g->owner : NULL;
  HGLOBAL mem = NULL;
  STATSTG st;
  const unsigned char *bytes;
  char *b64;
  char *url;
  size_t n;
  if ((This->g == NULL) || This->g->closed) {
    return S_OK;
  }
  if (FAILED(hr) || (This->stream == NULL)) {
    wvg_settle_str(w, This->req, 0, "capture failed");
    return S_OK;
  }
  ZeroMemory(&st, sizeof(st));
  if (FAILED(This->stream->lpVtbl->Stat(This->stream, &st, STATFLAG_NONAME)) ||
      FAILED(GetHGlobalFromStream(This->stream, &mem)) || (mem == NULL) ||
      (st.cbSize.QuadPart == 0) ||
      (st.cbSize.QuadPart > (ULONGLONG)GlobalSize(mem))) {
    wvg_settle_str(w, This->req, 0, "capture failed");
    return S_OK;
  }
  bytes = (const unsigned char *)GlobalLock(mem);
  b64 = (bytes != NULL) ? wvg_base64(bytes, (size_t)st.cbSize.QuadPart) : NULL;
  GlobalUnlock(mem);
  if (b64 == NULL) {
    wvg_settle_str(w, This->req, 0, "out of memory");
    return S_OK;
  }
  n = strlen(b64) + 32;
  url = (char *)malloc(n);
  if (url != NULL) {
    snprintf(url, n, "data:image/%s;base64,%s", This->jpeg ? "jpeg" : "png",
             b64);
    wvg_settle_str(w, This->req, 1, url);
    free(url);
  }
  free(b64);
  return S_OK;
}

WVG_VTBL(wvg_vtbl_capture, wvg_on_capture,
         &IID_ICoreWebView2CapturePreviewCompletedHandler);

static void wvg_capture(struct webview_guest *g, int req, int jpeg) {
  wvg_obj *o;
  HRESULT hr;
  if (g->view == NULL) {
    wvg_settle_str(g->owner, req, 0, "not loaded");
    return;
  }
  o = wvg_obj_new(&wvg_vtbl_capture, g, req);
  if (o == NULL) {
    return;
  }
  if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &o->stream))) {
    o->stream = NULL;
    o->lpVtbl->Release(o);
    wvg_settle_str(g->owner, req, 0, "no stream");
    return;
  }
  o->req = req;
  o->jpeg = jpeg;
  hr = g->view->lpVtbl->CapturePreview(
      g->view,
      jpeg ? COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_JPEG
           : COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG,
      o->stream,
      (ICoreWebView2CapturePreviewCompletedHandler *)o);
  o->lpVtbl->Release(o);
  if (FAILED(hr)) {
    wvg_settle_str(g->owner, req, 0, "CapturePreview failed");
  }
}

/* --------------------------------------------------------------------------
 * executeScript() on a guest, answer goes back to the host page
 * ------------------------------------------------------------------------*/
struct wvg_exec_ctx {
  struct webview_guest *g; /* holds a reference */
  int req;
};

static void wvg_exec_done(void *userdata, int failed, const char *json) {
  struct wvg_exec_ctx *ctx = (struct wvg_exec_ctx *)userdata;
  if (!ctx->g->closed) {
    if (failed) {
      wvg_settle_str(ctx->g->owner, ctx->req, 0, "script failed");
    } else {
      wvg_settle(ctx->g->owner, ctx->req, 1, json);
    }
  }
  webview_guest_release(ctx->g);
  free(ctx);
}

static void wvg_exec(struct webview_guest *g, int req, const char *js) {
  struct wvg_exec_ctx *ctx;
  if (g->view == NULL) {
    wvg_settle_str(g->owner, req, 0, "the page is not loaded yet");
    return;
  }
  ctx = (struct wvg_exec_ctx *)malloc(sizeof(*ctx));
  if (ctx == NULL) {
    return;
  }
  ctx->g = g;
  ctx->req = req;
  InterlockedIncrement(&g->ref);
  if (WebView2ExecuteOn(g->view, g->owner->priv.hwnd, js, &wvg_exec_done,
                        ctx) != 0) {
    webview_guest_release(g);
    free(ctx);
    wvg_settle_str(g->owner, req, 0, "ExecuteScript failed");
  }
}

/* --------------------------------------------------------------------------
 * Requests from the host page
 *
 *   __wvg \x1f op \x1f id \x1f req \x1f arg1 \x1f arg2 ... \x1f last
 *
 * The LAST argument of every op is taken verbatim to the end of the string,
 * so a script can contain any character at all.
 * ------------------------------------------------------------------------*/

/* "1.25" -> 1.25 whatever the C locale says the decimal point is (atof
   would read "1.25" as 1 in a comma locale after setlocale(LC_ALL, "")). */
static double wvg_parse_number(const char *s) {
  double v = 0.0;
  double scale = 0.1;
  if ((s == NULL) || (*s == '\0')) {
    return 1.0;
  }
  while ((*s >= '0') && (*s <= '9')) {
    v = v * 10.0 + (double)(*s++ - '0');
  }
  if (*s == '.') {
    s++;
    while ((*s >= '0') && (*s <= '9')) {
      v += (double)(*s++ - '0') * scale;
      scale /= 10.0;
    }
  }
  return v;
}

/* Split off up to `max` fields; the last one keeps the rest of the string. */
static int wvg_split(char *s, char **fields, int max) {
  int n = 0;
  while ((s != NULL) && (n < max)) {
    fields[n++] = s;
    if (n == max) {
      break;
    }
    s = strchr(s, '\x1f');
    if (s != NULL) {
      *s++ = '\0';
    }
  }
  return n;
}

/* --------------------------------------------------------------------------
 * Which host pages may use the tag
 *
 * A <webview> can open any site in any partition and read it back, so only
 * YOUR pages get to create one. By default that is anything local:
 * file://, about:blank (webview_set_html), data:, and http(s) on localhost /
 * 127.0.0.1 / [::1]. If your app's UI is served from somewhere else, allow
 * it with webview_guest_allow_origin("https://app.example.com"). If the main
 * window ever shows a remote site, that site cannot touch the tag.
 * ------------------------------------------------------------------------*/
static char **wvg_origins = NULL;
static int wvg_origin_count = 0;
static int wvg_allow_any_origin = 0;

WEBVIEW_API int webview_guest_allow_origin(const char *origin) {
  char **grown;
  if ((origin == NULL) || (origin[0] == '\0')) {
    return -1;
  }
  if (strcmp(origin, "*") == 0) {
    wvg_allow_any_origin = 1; /* only if you really mean it */
    return 0;
  }
  grown = (char **)realloc(wvg_origins,
                           sizeof(char *) * (size_t)(wvg_origin_count + 1));
  if (grown == NULL) {
    return -1;
  }
  wvg_origins = grown;
  wvg_origins[wvg_origin_count] = webview_strdup(origin);
  if (wvg_origins[wvg_origin_count] == NULL) {
    return -1;
  }
  wvg_origin_count++;
  return 0;
}

/* "https://host[:port]" matches `url` exactly up to the origin's end */
static int wvg_origin_matches(const char *url, const char *origin) {
  size_t n = strlen(origin);
  while ((n > 0) && (origin[n - 1] == '/')) {
    n--;
  }
  if (_strnicmp(url, origin, n) != 0) {
    return 0;
  }
  return (url[n] == '\0') || (url[n] == '/') || (url[n] == '?') ||
         (url[n] == '#');
}

static int wvg_is_local_host(const char *url) {
  const char *p = strstr(url, "://");
  const char *hosts[] = {"localhost", "127.0.0.1", "[::1]"};
  int i;
  if (p == NULL) {
    return 0;
  }
  if ((_strnicmp(url, "http://", 7) != 0) &&
      (_strnicmp(url, "https://", 8) != 0)) {
    return 0;
  }
  p += 3;
  for (i = 0; i < 3; i++) {
    size_t n = strlen(hosts[i]);
    if ((_strnicmp(p, hosts[i], n) == 0) &&
        ((p[n] == '\0') || (p[n] == ':') || (p[n] == '/'))) {
      return 1;
    }
  }
  return 0;
}

static int wvg_origin_allowed(const char *url) {
  int i;
  if (wvg_allow_any_origin) {
    return 1;
  }
  if ((url == NULL) || (url[0] == '\0')) {
    return 0;
  }
  if ((_strnicmp(url, "file:", 5) == 0) || (_strnicmp(url, "about:", 6) == 0) ||
      (_strnicmp(url, "data:", 5) == 0) || wvg_is_local_host(url)) {
    return 1;
  }
  for (i = 0; i < wvg_origin_count; i++) {
    if (wvg_origin_matches(url, wvg_origins[i])) {
      return 1;
    }
  }
  return 0;
}

/* Returns 1 if the message was a <webview> request and has been handled. */
static int webview_guest_on_host_message(struct webview *w,
                                         const char *message) {
  char *copy;
  char *f[8];
  int n;
  const char *op;
  int id;
  int req;
  struct webview_guest *g;
  if ((message == NULL) ||
      (strncmp(message, WVG_PREFIX, sizeof(WVG_PREFIX) - 1) != 0)) {
    return 0;
  }
  if ((w->priv.webview2 == NULL) ||
      !wvg_origin_allowed(w->priv.webview2->message_source)) {
    static int warned = 0;
    if (!warned) {
      warned = 1;
      webview_print_log("<webview> refused: this page's origin is not allowed "
                        "(see webview_guest_allow_origin):");
      webview_print_log(w->priv.webview2 != NULL
                            ? w->priv.webview2->message_source
                            : "(no page)");
    }
    /* tell the element, so it shows its fallback instead of hanging */
    if (strncmp(message, WVG_PREFIX "create" "\x1f",
                sizeof(WVG_PREFIX "create") ) == 0) {
      char msg[200];
      snprintf(msg, sizeof(msg),
               "{\"__wvg\":\"ev\",\"id\":%d,\"type\":\"unsupported\","
               "\"detail\":{\"reason\":\"origin not allowed\"}}",
               atoi(message + sizeof(WVG_PREFIX "create")));
      wvg_host_post(w, msg);
    }
    return 1;
  }
  copy = webview_strdup(message + sizeof(WVG_PREFIX) - 1);
  if (copy == NULL) {
    return 1;
  }
  memset(f, 0, sizeof(f));
  n = wvg_split(copy, f, 3);
  if (n < 3) {
    free(copy);
    return 1;
  }
  op = f[0];
  id = atoi(f[1]);
  /* f[2] = "req" or "req\x1frest" */
  {
    char *rest = strchr(f[2], '\x1f');
    if (rest != NULL) {
      *rest++ = '\0';
    }
    req = atoi(f[2]);
    f[3] = rest; /* may be NULL */
  }

  if (strcmp(op, "reset") == 0) {
    /* A new host document: every guest of the old one is gone. Done right
       here, not later, so it cannot overtake the new document's "create"s.
       Closing OTHER controllers from inside the host's message handler is
       fine; each guest only ever closes itself through here. */
    webview_guests_close_all(w);
    free(copy);
    return 1;
  }
  if (strcmp(op, "create") == 0) {
    char *a[3] = {NULL, NULL, NULL};
    if (f[3] != NULL) {
      wvg_split(f[3], a, 3);
    }
    /* src \x1f partition \x1f flags */
    wvg_create(w, id, a[0], a[1], a[2]);
    free(copy);
    return 1;
  }

  if (strcmp(op, "order") == 0) {
    /* not about one guest: the stacking of all of them */
    wvg_apply_order(w, f[3]);
    free(copy);
    return 1;
  }

  g = wvg_find(w, id);
  if (g == NULL) {
    wvg_settle_str(w, req, 0, "no such webview");
    free(copy);
    return 1;
  }

  if (strcmp(op, "remove") == 0) {
    wvg_close(g);
  } else if (strcmp(op, "bounds") == 0) {
    int v[9] = {0};
    if ((f[3] != NULL) && (wvg_read_ints(f[3], v, 9, NULL) == 9)) {
      wvg_parse_holes(g, strchr(f[3], ';'));
      g->x = v[0];
      g->y = v[1];
      g->w = v[2];
      g->h = v[3];
      g->cx = v[4];
      g->cy = v[5];
      g->cw = v[6];
      g->ch = v[7];
      g->visible = v[8];
      wvg_apply_bounds(g);
    }
  } else if (strcmp(op, "freeze") == 0) {
    g->frozen = (f[3] != NULL) && (f[3][0] == '1');
    wvg_apply_bounds(g);
    wvg_settle(w, req, 1, "null");
  } else if (strcmp(op, "nav") == 0) {
    free(g->src);
    g->src = webview_strdup(f[3] != NULL ? f[3] : "");
    wvg_navigate(g, g->src);
  } else if (strcmp(op, "back") == 0) {
    if (g->view != NULL) g->view->lpVtbl->GoBack(g->view);
  } else if (strcmp(op, "forward") == 0) {
    if (g->view != NULL) g->view->lpVtbl->GoForward(g->view);
  } else if (strcmp(op, "reload") == 0) {
    if (g->view != NULL) g->view->lpVtbl->Reload(g->view);
  } else if (strcmp(op, "stop") == 0) {
    if (g->view != NULL) g->view->lpVtbl->Stop(g->view);
  } else if (strcmp(op, "devtools") == 0) {
    if (g->view != NULL) g->view->lpVtbl->OpenDevToolsWindow(g->view);
  } else if (strcmp(op, "focus") == 0) {
    if (g->controller != NULL)
      g->controller->lpVtbl->MoveFocus(
          g->controller, COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
  } else if (strcmp(op, "zoom") == 0) {
    double z = wvg_parse_number(f[3]);
    if ((g->controller != NULL) && (z > 0.05) && (z < 20.0)) {
      g->controller->lpVtbl->put_ZoomFactor(g->controller, z);
    }
  } else if (strcmp(op, "exec") == 0) {
    wvg_exec(g, req, f[3] != NULL ? f[3] : "undefined");
  } else if (strcmp(op, "script") == 0) {
    const char *js = (f[3] != NULL) ? f[3] : "";
    if (g->view != NULL) {
      if (wvg_add_script(g, js, req) != 0) {
        wvg_settle_str(w, req, 0, "AddScriptToExecuteOnDocumentCreated failed");
      }
    } else {
      /* not created yet: keep it, it goes in before the first page */
      char **grown = (char **)realloc(
          g->scripts, sizeof(char *) * (size_t)(g->script_count + 1));
      if (grown != NULL) {
        g->scripts = grown;
        g->scripts[g->script_count++] = webview_strdup(js);
      }
      wvg_settle(w, req, 1, "null");
    }
  } else if (strcmp(op, "post") == 0) {
    if ((g->view != NULL) && (f[3] != NULL)) {
      WCHAR *wj = webview_to_utf16(f[3]);
      if (wj != NULL) {
        g->view->lpVtbl->PostWebMessageAsJson(g->view, wj);
        GlobalFree(wj);
      }
    }
  } else if (strcmp(op, "look") == 0) {
    /* opacity (0..1000) \x1f mode (0 opaque, 1 transparent, 2 force) */
    char *a[2] = {NULL, NULL};
    if (f[3] != NULL) {
      wvg_split(f[3], a, 2);
    }
    wvg_apply_look(g, a[0] != NULL ? atoi(a[0]) : 1000,
                   a[1] != NULL ? atoi(a[1]) : 0);
  } else if (strcmp(op, "capture") == 0) {
    /* JPEG is several times smaller - what the live backdrop uses */
    wvg_capture(g, req, (f[3] != NULL) && (strcmp(f[3], "jpeg") == 0));
  } else {
    wvg_settle_str(w, req, 0, "unknown operation");
  }
  free(copy);
  return 1;
}

/* The host page is being replaced by a new document - including an error
   page, where no script of ours runs to say "reset". Every guest belonged to
   the old document. WebView2 raises this before any script of the new
   document runs, so it cannot swallow the new page's own guests. */
static HRESULT STDMETHODCALLTYPE wvg_on_host_content_loading(
    wvg_obj *This, ICoreWebView2 *sender,
    ICoreWebView2ContentLoadingEventArgs *args) {
  (void)sender;
  (void)args;
  if (This->w != NULL) {
    webview_guests_close_all(This->w);
  }
  return S_OK;
}

WVG_VTBL(wvg_vtbl_host_content_loading, wvg_on_host_content_loading,
         &IID_ICoreWebView2ContentLoadingEventHandler);

/* Called from the host controller callback: the <webview> tag itself. */
static void webview_guest_install_host_script(webview2 *pwv2) {
#if !defined(WEBVIEW_NO_GUEST_TAG)
  WCHAR *wjs;
  wvg_obj *o;
  EventRegistrationToken token;
  if ((pwv2 == NULL) || (pwv2->webview == NULL)) {
    return;
  }
  o = wvg_obj_new(&wvg_vtbl_host_content_loading, NULL, 0);
  if (o != NULL) {
    o->w = (struct webview *)pwv2->context;
    pwv2->webview->lpVtbl->add_ContentLoading(
        pwv2->webview, (ICoreWebView2ContentLoadingEventHandler *)o, &token);
    o->lpVtbl->Release(o);
  }
  wjs = webview_to_utf16(WEBVIEW_JS_GUEST_HOST);
  if (wjs != NULL) {
    pwv2->webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(
        pwv2->webview, wjs, NULL);
    GlobalFree(wjs);
  }
#else
  (void)pwv2;
#endif
}

/* --------------------------------------------------------------------------
 * C API
 * ------------------------------------------------------------------------*/

struct wvg_c_eval_ctx {
  struct webview *w;
  webview_eval_cb_t cb;
  void *userdata;
};

static void wvg_c_eval_done(void *userdata, int failed, const char *json) {
  struct wvg_c_eval_ctx *ctx = (struct wvg_c_eval_ctx *)userdata;
  if (ctx->cb != NULL) {
    ctx->cb(ctx->w, failed ? "null" : json, ctx->userdata);
  }
  free(ctx);
}

WEBVIEW_API int webview_guest_eval(struct webview *w, int guest_id,
                                   const char *js, webview_eval_cb_t cb,
                                   void *userdata) {
  struct webview_guest *g = wvg_find(w, guest_id);
  struct wvg_c_eval_ctx *ctx;
  if ((g == NULL) || (g->view == NULL) || (js == NULL)) {
    return -1;
  }
  ctx = (struct wvg_c_eval_ctx *)malloc(sizeof(*ctx));
  if (ctx == NULL) {
    return -1;
  }
  ctx->w = w;
  ctx->cb = cb;
  ctx->userdata = userdata;
  if (WebView2ExecuteOn(g->view, w->priv.hwnd, js, &wvg_c_eval_done, ctx) !=
      0) {
    free(ctx);
    return -1;
  }
  return 0;
}

WEBVIEW_API int webview_guest_navigate(struct webview *w, int guest_id,
                                       const char *url) {
  struct webview_guest *g = wvg_find(w, guest_id);
  if ((g == NULL) || (url == NULL)) {
    return -1;
  }
  free(g->src);
  g->src = webview_strdup(url);
  wvg_navigate(g, g->src);
  return 0;
}
