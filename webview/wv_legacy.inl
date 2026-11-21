/* ---------------------------------------------------------------------------
 * wv_legacy.inl - modern JavaScript on the IE fallback
 *
 * The MSHTML backend runs on whatever Internet Explorer the system has: IE6
 * on Windows 2000, IE8 on XP, IE9 on Vista, IE11 on 7. None of them knows
 * ES2015. Here every script on its way into the control goes through the
 * translator (wv_downlevel.inl), and the runtime it needs
 * (js/legacy_runtime.js, compiled in through wv_legacy_js.h) goes in front
 * of the page's first script:
 *
 *   - pages and .js files the control loads (file:, http:, res:, ...): an
 *     urlmon MIME filter rewrites text/html and the JavaScript types on the
 *     fly, for the whole process;
 *   - webview_set_html(): the html is rewritten before it is written;
 *   - webview_eval(): the code is translated, and the runtime installed in
 *     the document first if the result needs it.
 *
 * F12 opens devtools drawn into the page (js/legacy_devtools.js): a console
 * with the page's log and a line that runs modern code, and an element
 * picker with a style editor. It follows w.browser.devtools / w.debug.
 *
 * Inline <script> blocks and .js files are translated; a script that is
 * plain ES3 already is left exactly as it was. type="module" scripts become
 * classic ones (IE ignores modules) when they import nothing.
 *
 * webview_set_legacy_js(0) turns all of it off.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#include "wv_legacy_js.h"

static int webview_legacy_js_on = 1;

WEBVIEW_API void webview_set_legacy_js(int enabled) {
  webview_legacy_js_on = enabled ? 1 : 0;
}

/* --------------------------------------------------------------- buffers */

struct webview_lbuf {
  char *p;
  size_t len, cap;
  int failed;
};

static void webview_lbuf_put(struct webview_lbuf *b, const char *s, size_t n) {
  if (b->failed) {
    return;
  }
  if (b->len + n + 1 > b->cap) {
    size_t cap = b->cap ? b->cap * 2 : 4096;
    char *p;
    while (cap < b->len + n + 1) {
      cap *= 2;
    }
    p = (char *)realloc(b->p, cap);
    if (p == NULL) {
      b->failed = 1;
      return;
    }
    b->p = p;
    b->cap = cap;
  }
  memcpy(b->p + b->len, s, n);
  b->len += n;
  b->p[b->len] = 0;
}

static void webview_lbuf_puts(struct webview_lbuf *b, const char *s) {
  webview_lbuf_put(b, s, strlen(s));
}

static int webview_lower(int c) {
  return ((c >= 'A') && (c <= 'Z')) ? c + 32 : c;
}

/* does s (n bytes left) start with `word`, any case? */
static int webview_ci_starts(const char *s, size_t n, const char *word) {
  size_t i, m = strlen(word);
  if (n < m) {
    return 0;
  }
  for (i = 0; i < m; i++) {
    if (webview_lower((unsigned char)s[i]) != word[i]) {
      return 0;
    }
  }
  return 1;
}

static size_t webview_ci_find(const char *s, size_t from, size_t n,
                              const char *word) {
  size_t i;
  for (i = from; i < n; i++) {
    if (webview_ci_starts(s + i, n - i, word)) {
      return i;
    }
  }
  return n;
}

/* ---------------------------------------------------------- one script */

/* the translation, or NULL to keep the script as it is (it was ES3, or it
   could not be translated: the reason goes to the log) */
static char *webview_legacy_script(const char *js, size_t len,
                                   const char *where) {
  char err[256];
  char *out;
  if (!webview_legacy_js_on || (len == 0)) {
    return NULL;
  }
  out = webview_downlevel(js, len, err, sizeof(err));
  if (out == NULL) {
    char msg[400];
    snprintf(msg, sizeof(msg), "legacy js: %s kept as it is: %s",
             where ? where : "script", err);
    webview_print_log(msg);
    return NULL;
  }
  if ((strlen(out) == len) && (memcmp(out, js, len) == 0)) {
    free(out); /* plain ES3 already */
    return NULL;
  }
  return out;
}

/* translated text inside a <script> block: no "</script" in it */
static void webview_lbuf_script(struct webview_lbuf *b, const char *s) {
  size_t n = strlen(s), i, from = 0;
  for (i = 0; i + 8 <= n; i++) {
    if ((s[i] == '<') && webview_ci_starts(s + i, n - i, "</script")) {
      webview_lbuf_put(b, s + from, i - from);
      webview_lbuf_puts(b, "<\\/");
      from = i + 2;
    }
  }
  webview_lbuf_put(b, s + from, n - from);
}

/* ------------------------------------------------------------- html */

struct webview_script_tag {
  size_t end;                   /* just after '>' */
  size_t type_at, type_len;     /* the type value, if any */
  int has_type, has_src, module, js;
};

static int webview_is_js_type(const char *t, size_t n) {
  static const char *const types[] = {
      "text/javascript", "application/javascript", "application/x-javascript",
      "text/ecmascript", "application/ecmascript", "text/jscript",
      "text/livescript", NULL};
  size_t i;
  while ((n > 0) && ((*t == ' ') || (*t == '\t'))) {
    t++;
    n--;
  }
  while ((n > 0) && ((t[n - 1] == ' ') || (t[n - 1] == '\t'))) {
    n--;
  }
  if (n == 0) {
    return 1;
  }
  for (i = 0; types[i] != NULL; i++) {
    if ((strlen(types[i]) == n) && webview_ci_starts(t, n, types[i])) {
      return 1;
    }
  }
  return 0;
}

/* the attributes of <script ...> starting at `at` ("<script") */
static int webview_parse_script_tag(const char *h, size_t at, size_t n,
                                    struct webview_script_tag *t) {
  size_t i = at + 7;
  memset(t, 0, sizeof(*t));
  while (i < n) {
    size_t name, name_len, value = 0, value_len = 0;
    while ((i < n) && strchr(" \t\r\n\f/", h[i]) && h[i]) {
      i++;
    }
    if (i >= n) {
      return 0;
    }
    if (h[i] == '>') {
      t->end = i + 1;
      t->js = !t->has_type || t->module ||
              webview_is_js_type(h + t->type_at, t->type_len);
      return 1;
    }
    name = i;
    while ((i < n) && !strchr(" \t\r\n\f/=>", h[i])) {
      i++;
    }
    name_len = i - name;
    while ((i < n) && strchr(" \t\r\n\f", h[i]) && h[i]) {
      i++;
    }
    if ((i < n) && (h[i] == '=')) {
      i++;
      while ((i < n) && strchr(" \t\r\n\f", h[i]) && h[i]) {
        i++;
      }
      if ((i < n) && ((h[i] == '"') || (h[i] == '\''))) {
        char q = h[i++];
        value = i;
        while ((i < n) && (h[i] != q)) {
          i++;
        }
        value_len = i - value;
        i++;
      } else {
        value = i;
        while ((i < n) && !strchr(" \t\r\n\f>", h[i])) {
          i++;
        }
        value_len = i - value;
      }
    }
    if ((name_len == 4) && webview_ci_starts(h + name, 4, "type")) {
      t->has_type = 1;
      t->type_at = value;
      t->type_len = value_len;
      t->module = (value_len == 6) && webview_ci_starts(h + value, 6, "module");
    } else if ((name_len == 3) && webview_ci_starts(h + name, 3, "src")) {
      t->has_src = 1;
    }
  }
  return 0;
}

/*
 * The page with its inline scripts translated and the runtime in front of
 * the first script (NULL when there is nothing to change). Pure C: tested
 * on its own.
 */
static char *webview_legacy_html(const char *h, size_t n, size_t *out_len) {
  struct webview_lbuf b;
  size_t i = 0, copied = 0;
  int changed = 0, runtime = 0;
  memset(&b, 0, sizeof(b));
  while (i < n) {
    struct webview_script_tag t;
    size_t content, close, after;
    if (h[i] != '<') {
      i++;
      continue;
    }
    if (webview_ci_starts(h + i, n - i, "<!--")) {
      size_t e = i + 4;
      while ((e + 3 <= n) && memcmp(h + e, "-->", 3) != 0) {
        e++;
      }
      i = e + 3;
      continue;
    }
    if (!webview_ci_starts(h + i, n - i, "<script") || (i + 7 >= n) ||
        !strchr(" \t\r\n\f/>", h[i + 7]) || !h[i + 7] ||
        !webview_parse_script_tag(h, i, n, &t)) {
      i++;
      continue;
    }
    content = t.end;
    close = webview_ci_find(h, content, n, "</script");
    after = (close < n) ? close + 8 : n;
    while ((after < n) && (h[after] != '>')) {
      after++;
    }
    if (after < n) {
      after++;
    }
    if (!t.js) { /* a template, JSON, ... */
      i = after;
      continue;
    }
    webview_lbuf_put(&b, h + copied, i - copied);
    copied = i;
    if (!runtime) {
      webview_lbuf_puts(&b, "<script>");
      webview_lbuf_puts(&b, WEBVIEW_JS_LEGACY_RUNTIME);
      webview_lbuf_puts(&b, "</script>");
      runtime = 1;
      changed = 1;
    }
    {
      char *js = (!t.has_src && (close > content))
                     ? webview_legacy_script(h + content, close - content,
                                             "inline <script>")
                     : NULL;
      if (t.module && (t.has_src || (js != NULL))) {
        /* type="module" -> a classic script IE runs */
        webview_lbuf_put(&b, h + i, t.type_at - i);
        webview_lbuf_puts(&b, "text/javascript");
        webview_lbuf_put(&b, h + t.type_at + t.type_len,
                         content - (t.type_at + t.type_len));
        copied = content;
        changed = 1;
      }
      if (js != NULL) {
        webview_lbuf_put(&b, h + copied, content - copied);
        webview_lbuf_script(&b, js);
        free(js);
        copied = close;
        changed = 1;
      }
    }
    i = after;
  }
  if (!changed || b.failed) {
    free(b.p);
    return NULL;
  }
  webview_lbuf_put(&b, h + copied, n - copied);
  if (b.failed) {
    free(b.p);
    return NULL;
  }
  if (out_len != NULL) {
    *out_len = b.len;
  }
  return b.p;
}

#ifdef _WIN32

/* ------------------------------------------------------------- IE version */

/* 6, 7, 8, 9, 10, 11 - the IE the control will be (0 if unknown) */
static int webview_ie_major(void) {
  static const char *const names[] = {"svcVersion", "Version", NULL};
  HKEY key;
  int major = 0, i;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\Internet Explorer", 0, KEY_READ,
                    &key) != ERROR_SUCCESS) {
    return 0;
  }
  for (i = 0; (names[i] != NULL) && (major == 0); i++) {
    char value[64];
    DWORD size = sizeof(value) - 1, type = 0;
    if (RegQueryValueExA(key, names[i], NULL, &type, (BYTE *)value, &size) ==
            ERROR_SUCCESS &&
        (type == REG_SZ)) {
      value[size] = 0;
      major = atoi(value);
      /* IE10 and 11 keep "9.10..." / "9.11..." in Version */
      if ((major == 9) && (strncmp(value, "9.1", 3) == 0)) {
        major = atoi(value + 2);
      }
    }
  }
  RegCloseKey(key);
  return major;
}

/* -------------------------------------------------------------- MIME filter */

/*
 * An urlmon MIME filter sits between a protocol (file:, http:, ...) and
 * MSHTML: it reads everything the protocol delivers, rewrites it, and
 * hands the result on. Registered once, it sees every load in the process.
 */
static const IID webview_IID_IInternetProtocolRoot = {
    0x79EAC9E3, 0xBAF9, 0x11CE, {0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B}};
static const IID webview_IID_IInternetProtocol = {
    0x79EAC9E4, 0xBAF9, 0x11CE, {0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B}};
static const IID webview_IID_IInternetProtocolSink = {
    0x79EAC9E5, 0xBAF9, 0x11CE, {0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B}};
static const IID webview_IID_IWinInetInfo = {
    0x79EAC9D6, 0xBAFA, 0x11CE, {0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B}};
static const IID webview_IID_IWinInetHttpInfo = {
    0x79EAC9D8, 0xBAFA, 0x11CE, {0x8C, 0x82, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B}};
static const IID webview_IID_IUnknown = {
    0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const IID webview_IID_IClassFactory = {
    0x00000001, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
/* made up for this filter */
static const CLSID webview_CLSID_html_filter = {
    0x5E1D2F3A, 0x6B7C, 0x4D8E, {0x9F, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xC1}};
static const CLSID webview_CLSID_js_filter = {
    0x5E1D2F3A, 0x6B7C, 0x4D8E, {0x9F, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xC2}};

typedef struct webview_filter {
  IInternetProtocolVtbl *lpVtbl;         /* what MSHTML reads from */
  IInternetProtocolSinkVtbl *sink_vtbl;  /* what the protocol reports to */
  LONG refs;
  int html;
  IInternetProtocolSink *up;   /* MSHTML's side */
  IInternetProtocol *down;     /* the protocol */
  struct webview_lbuf in;
  char *out;
  size_t out_len, pos;
  int done;
  char where[200];
} webview_filter;

#define WEBVIEW_FILTER_OF_SINK(p)                                              \
  ((webview_filter *)((char *)(p)-offsetof(webview_filter, sink_vtbl)))

static ULONG STDMETHODCALLTYPE WvFilter_AddRef(IInternetProtocol *This) {
  return (ULONG)InterlockedIncrement(&((webview_filter *)This)->refs);
}

static void webview_filter_drop(webview_filter *f) {
  if (f->up != NULL) {
    f->up->lpVtbl->Release(f->up);
    f->up = NULL;
  }
  if (f->down != NULL) {
    f->down->lpVtbl->Release(f->down);
    f->down = NULL;
  }
}

static ULONG STDMETHODCALLTYPE WvFilter_Release(IInternetProtocol *This) {
  webview_filter *f = (webview_filter *)This;
  LONG r = InterlockedDecrement(&f->refs);
  if (r == 0) {
    webview_filter_drop(f);
    if (f->out != f->in.p) {
      free(f->out);
    }
    free(f->in.p);
    free(f);
  }
  return (ULONG)r;
}

static HRESULT STDMETHODCALLTYPE WvFilter_QueryInterface(IInternetProtocol *This,
                                                         REFIID riid,
                                                         void **ppv) {
  webview_filter *f = (webview_filter *)This;
  if (iid_eq(riid, &webview_IID_IUnknown) ||
      iid_eq(riid, &webview_IID_IInternetProtocolRoot) ||
      iid_eq(riid, &webview_IID_IInternetProtocol)) {
    *ppv = This;
  } else if (iid_eq(riid, &webview_IID_IInternetProtocolSink)) {
    *ppv = &f->sink_vtbl;
  } else if ((f->down != NULL) &&
             (iid_eq(riid, &webview_IID_IWinInetInfo) ||
              iid_eq(riid, &webview_IID_IWinInetHttpInfo))) {
    /* http headers and status: the protocol's own */
    return f->down->lpVtbl->QueryInterface(f->down, riid, ppv);
  } else {
    *ppv = NULL;
    return E_NOINTERFACE;
  }
  WvFilter_AddRef(This);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Start(IInternetProtocol *This,
                                                LPCWSTR url,
                                                IInternetProtocolSink *sink,
                                                IInternetBindInfo *info,
                                                DWORD flags,
                                                HANDLE_PTR reserved) {
  webview_filter *f = (webview_filter *)This;
  PROTOCOLFILTERDATA *data = (PROTOCOLFILTERDATA *)reserved;
  (void)info;
  if (!(flags & PI_FILTER_MODE) || (data == NULL) || (data->pProtocol == NULL) ||
      (sink == NULL)) {
    return E_INVALIDARG;
  }
  f->up = sink;
  sink->lpVtbl->AddRef(sink);
  f->down = data->pProtocol;
  f->down->lpVtbl->AddRef(f->down);
  f->where[0] = 0;
  if (url != NULL) {
    WideCharToMultiByte(CP_UTF8, 0, url, -1, f->where, sizeof(f->where) - 1,
                        NULL, NULL);
    f->where[sizeof(f->where) - 1] = 0;
  }
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Continue(IInternetProtocol *This,
                                                   PROTOCOLDATA *data) {
  webview_filter *f = (webview_filter *)This;
  return (f->down != NULL) ? f->down->lpVtbl->Continue(f->down, data) : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Abort(IInternetProtocol *This,
                                                HRESULT reason, DWORD options) {
  webview_filter *f = (webview_filter *)This;
  return (f->down != NULL) ? f->down->lpVtbl->Abort(f->down, reason, options)
                           : S_OK;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Terminate(IInternetProtocol *This,
                                                    DWORD options) {
  webview_filter *f = (webview_filter *)This;
  if (f->down != NULL) {
    f->down->lpVtbl->Terminate(f->down, options);
  }
  webview_filter_drop(f);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Suspend(IInternetProtocol *This) {
  (void)This;
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Resume(IInternetProtocol *This) {
  (void)This;
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Read(IInternetProtocol *This,
                                               void *pv, ULONG cb,
                                               ULONG *read) {
  webview_filter *f = (webview_filter *)This;
  size_t left, n;
  if (read != NULL) {
    *read = 0;
  }
  if (!f->done) {
    return E_PENDING;
  }
  left = f->out_len - f->pos;
  if (left == 0) {
    return S_FALSE;
  }
  n = (left < cb) ? left : cb;
  memcpy(pv, f->out + f->pos, n);
  f->pos += n;
  if (read != NULL) {
    *read = (ULONG)n;
  }
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE WvFilter_Seek(IInternetProtocol *This,
                                               LARGE_INTEGER move, DWORD origin,
                                               ULARGE_INTEGER *pos) {
  (void)This;
  (void)move;
  (void)origin;
  (void)pos;
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE WvFilter_LockRequest(IInternetProtocol *This,
                                                      DWORD options) {
  webview_filter *f = (webview_filter *)This;
  return (f->down != NULL) ? f->down->lpVtbl->LockRequest(f->down, options)
                           : S_OK;
}

static HRESULT STDMETHODCALLTYPE WvFilter_UnlockRequest(IInternetProtocol *This) {
  webview_filter *f = (webview_filter *)This;
  return (f->down != NULL) ? f->down->lpVtbl->UnlockRequest(f->down) : S_OK;
}

static IInternetProtocolVtbl webview_filter_vtbl = {
    WvFilter_QueryInterface, WvFilter_AddRef,      WvFilter_Release,
    WvFilter_Start,          WvFilter_Continue,    WvFilter_Abort,
    WvFilter_Terminate,      WvFilter_Suspend,     WvFilter_Resume,
    WvFilter_Read,           WvFilter_Seek,        WvFilter_LockRequest,
    WvFilter_UnlockRequest};

/* all of it is in: rewrite, then hand it on in one piece */
static void webview_filter_finish(webview_filter *f) {
  char *changed = NULL;
  size_t changed_len = 0;
  if (f->done) {
    return;
  }
  f->done = 1;
  if (!f->in.failed && (f->in.len > 0)) {
    if (f->html) {
      changed = webview_legacy_html(f->in.p, f->in.len, &changed_len);
    } else {
      changed = webview_legacy_script(f->in.p, f->in.len, f->where);
      if (changed != NULL) {
        changed_len = strlen(changed);
      }
    }
  }
  if (changed != NULL) {
    f->out = changed;
    f->out_len = changed_len;
  } else {
    f->out = f->in.p;
    f->out_len = f->in.failed ? 0 : f->in.len;
  }
  if (f->up != NULL) {
    f->up->lpVtbl->ReportData(f->up,
                              BSCF_FIRSTDATANOTIFICATION |
                                  BSCF_LASTDATANOTIFICATION |
                                  BSCF_DATAFULLYAVAILABLE,
                              (ULONG)f->out_len, (ULONG)f->out_len);
    if (f->up != NULL) {
      f->up->lpVtbl->ReportResult(f->up, S_OK, 0, NULL);
    }
  }
}

/* read what the protocol has; 1 when it has no more */
static int webview_filter_pull(webview_filter *f) {
  char chunk[16384];
  for (;;) {
    ULONG got = 0;
    HRESULT hr;
    if (f->down == NULL) {
      return 1;
    }
    hr = f->down->lpVtbl->Read(f->down, chunk, sizeof(chunk), &got);
    if (got > 0) {
      webview_lbuf_put(&f->in, chunk, got);
    }
    if (hr == S_FALSE) {
      return 1;
    }
    if ((hr != S_OK) || (got == 0)) {
      return FAILED(hr) && (hr != E_PENDING);
    }
  }
}

static HRESULT STDMETHODCALLTYPE WvSink_QueryInterface(IInternetProtocolSink *This,
                                                       REFIID riid, void **ppv) {
  return WvFilter_QueryInterface((IInternetProtocol *)WEBVIEW_FILTER_OF_SINK(This),
                                 riid, ppv);
}

static ULONG STDMETHODCALLTYPE WvSink_AddRef(IInternetProtocolSink *This) {
  return WvFilter_AddRef((IInternetProtocol *)WEBVIEW_FILTER_OF_SINK(This));
}

static ULONG STDMETHODCALLTYPE WvSink_Release(IInternetProtocolSink *This) {
  return WvFilter_Release((IInternetProtocol *)WEBVIEW_FILTER_OF_SINK(This));
}

static HRESULT STDMETHODCALLTYPE WvSink_Switch(IInternetProtocolSink *This,
                                               PROTOCOLDATA *data) {
  webview_filter *f = WEBVIEW_FILTER_OF_SINK(This);
  return (f->up != NULL) ? f->up->lpVtbl->Switch(f->up, data) : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE WvSink_ReportProgress(IInternetProtocolSink *This,
                                                       ULONG status,
                                                       LPCWSTR text) {
  webview_filter *f = WEBVIEW_FILTER_OF_SINK(This);
  if (status == BINDSTATUS_CACHEFILENAMEAVAILABLE) {
    return S_OK; /* the file holds the old bytes: read ours */
  }
  return (f->up != NULL) ? f->up->lpVtbl->ReportProgress(f->up, status, text)
                         : S_OK;
}

static HRESULT STDMETHODCALLTYPE WvSink_ReportData(IInternetProtocolSink *This,
                                                   DWORD flags, ULONG progress,
                                                   ULONG max) {
  webview_filter *f = WEBVIEW_FILTER_OF_SINK(This);
  (void)progress;
  (void)max;
  if (f->done) {
    return S_OK;
  }
  if (webview_filter_pull(f) || (flags & BSCF_LASTDATANOTIFICATION)) {
    webview_filter_finish(f);
  }
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE WvSink_ReportResult(IInternetProtocolSink *This,
                                                     HRESULT result, DWORD error,
                                                     LPCWSTR text) {
  webview_filter *f = WEBVIEW_FILTER_OF_SINK(This);
  if (f->done) {
    return S_OK;
  }
  if (FAILED(result)) {
    f->done = 1; /* nothing to rewrite: pass the failure on */
    f->out = f->in.p;
    f->out_len = 0;
    return (f->up != NULL)
               ? f->up->lpVtbl->ReportResult(f->up, result, error, text)
               : S_OK;
  }
  webview_filter_pull(f);
  webview_filter_finish(f);
  return S_OK;
}

static IInternetProtocolSinkVtbl webview_filter_sink_vtbl = {
    WvSink_QueryInterface, WvSink_AddRef,     WvSink_Release,
    WvSink_Switch,         WvSink_ReportProgress, WvSink_ReportData,
    WvSink_ReportResult};

typedef struct webview_filter_factory {
  IClassFactoryVtbl *lpVtbl;
  int html;
} webview_filter_factory;

static HRESULT STDMETHODCALLTYPE WvFactory_QueryInterface(IClassFactory *This,
                                                          REFIID riid,
                                                          void **ppv) {
  if (iid_eq(riid, &webview_IID_IUnknown) ||
      iid_eq(riid, &webview_IID_IClassFactory)) {
    *ppv = This;
    return S_OK;
  }
  *ppv = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE WvFactory_AddRef(IClassFactory *This) {
  (void)This;
  return 2; /* static */
}

static ULONG STDMETHODCALLTYPE WvFactory_Release(IClassFactory *This) {
  (void)This;
  return 1;
}

static HRESULT STDMETHODCALLTYPE WvFactory_CreateInstance(IClassFactory *This,
                                                          IUnknown *outer,
                                                          REFIID riid,
                                                          void **ppv) {
  webview_filter *f;
  HRESULT hr;
  *ppv = NULL;
  if (outer != NULL) {
    return CLASS_E_NOAGGREGATION;
  }
  f = (webview_filter *)calloc(1, sizeof(webview_filter));
  if (f == NULL) {
    return E_OUTOFMEMORY;
  }
  f->lpVtbl = &webview_filter_vtbl;
  f->sink_vtbl = &webview_filter_sink_vtbl;
  f->refs = 1;
  f->html = ((webview_filter_factory *)This)->html;
  hr = WvFilter_QueryInterface((IInternetProtocol *)f, riid, ppv);
  WvFilter_Release((IInternetProtocol *)f);
  return hr;
}

static HRESULT STDMETHODCALLTYPE WvFactory_LockServer(IClassFactory *This,
                                                      BOOL lock) {
  (void)This;
  (void)lock;
  return S_OK;
}

static IClassFactoryVtbl webview_filter_factory_vtbl = {
    WvFactory_QueryInterface, WvFactory_AddRef, WvFactory_Release,
    WvFactory_CreateInstance, WvFactory_LockServer};

static webview_filter_factory webview_html_factory = {
    &webview_filter_factory_vtbl, 1};
static webview_filter_factory webview_js_factory = {
    &webview_filter_factory_vtbl, 0};

/* once per process, before the first page */
static void webview_legacy_register(void) {
  static int done = 0;
  static const wchar_t *const js_types[] = {
      L"application/javascript", L"application/x-javascript",
      L"text/javascript", L"application/ecmascript", L"text/ecmascript",
      L"text/jscript", NULL};
  typedef HRESULT(WINAPI * GetSessionFn)(DWORD, IInternetSession **, DWORD);
  HMODULE urlmon;
  GetSessionFn get_session;
  IInternetSession *session = NULL;
  int i, old_ie;
  if (done || !webview_legacy_js_on) {
    return;
  }
  done = 1;
  /* IE8 and older: no getters on plain objects, and IE6/7 no
     Element.prototype; the translator sends those through the runtime */
  old_ie = webview_ie_major();
  old_ie = (old_ie == 0) || (old_ie <= 8);
  if (old_ie) {
    webview_downlevel_mode(WEBVIEW_DOWNLEVEL_OLD_IE);
  }
  urlmon = LoadLibraryA("urlmon.dll");
  if (urlmon == NULL) {
    webview_print_log("legacy js: urlmon.dll not found");
    return;
  }
  get_session = (GetSessionFn)GetProcAddress(urlmon, "CoInternetGetSession");
  if ((get_session == NULL) || (get_session(0, &session, 0) != S_OK) ||
      (session == NULL)) {
    webview_print_log("legacy js: no urlmon session, pages load untranslated");
    return;
  }
  if (session->lpVtbl->RegisterMimeFilter(
          session, (IClassFactory *)&webview_html_factory,
          iid_unref(&webview_CLSID_html_filter), L"text/html") != S_OK) {
    webview_print_log("legacy js: could not register the html filter");
  }
  for (i = 0; js_types[i] != NULL; i++) {
    session->lpVtbl->RegisterMimeFilter(session,
                                        (IClassFactory *)&webview_js_factory,
                                        iid_unref(&webview_CLSID_js_filter),
                                        js_types[i]);
  }
  session->lpVtbl->Release(session);
  {
    char msg[160];
    snprintf(msg, sizeof(msg),
             "legacy js: on (IE %d), scripts are translated to ES3%s",
             webview_ie_major(),
             old_ie ? "; getters and element methods go through the runtime"
                    : "");
    webview_print_log(msg);
  }
}

/* --------------------------------------------------------------- eval */

/* eval(code) in the document's script engine; the result in *result if
   given (VariantClear it) */
static int webview_mshtml_run(IDispatch *script, const char *code,
                              VARIANT *result) {
  DISPID dispid;
  wchar_t *eval_name = L"eval";
  DISPPARAMS params;
  VARIANT arg, ignored;
  EXCEPINFO excep;
  UINT arg_err = (UINT)-1;
  wchar_t *wide;
  HRESULT hr;
  if (script->lpVtbl->GetIDsOfNames(script, iid_unref(&IID_NULL), &eval_name, 1,
                                    LOCALE_SYSTEM_DEFAULT, &dispid) != S_OK) {
    return -1;
  }
  wide = webview_to_utf16(code);
  if (wide == NULL) {
    return -1;
  }
  VariantInit(&arg);
  VariantInit(&ignored);
  arg.vt = VT_BSTR;
  arg.bstrVal = SysAllocString(wide);
  GlobalFree(wide);
  if (arg.bstrVal == NULL) {
    return -1;
  }
  params.cArgs = 1;
  params.cNamedArgs = 0;
  params.rgvarg = &arg;
  params.rgdispidNamedArgs = NULL;
  memset(&excep, 0, sizeof(excep));
  hr = script->lpVtbl->Invoke(script, dispid, iid_unref(&IID_NULL), 0,
                              DISPATCH_METHOD, &params,
                              (result != NULL) ? result : &ignored, &excep,
                              &arg_err);
  SysFreeString(arg.bstrVal);
  VariantClear(&ignored);
  if (excep.bstrSource != NULL) {
    SysFreeString(excep.bstrSource);
  }
  if (excep.bstrDescription != NULL) {
    SysFreeString(excep.bstrDescription);
  }
  if (excep.bstrHelpFile != NULL) {
    SysFreeString(excep.bstrHelpFile);
  }
  return (hr == S_OK) ? 0 : -1;
}

/* the runtime, if this document does not have it yet */
static void webview_mshtml_ensure_runtime(IDispatch *script) {
  VARIANT r;
  int missing = 1;
  VariantInit(&r);
  if ((webview_mshtml_run(script, "typeof __wv", &r) == 0) &&
      (r.vt == VT_BSTR) && (r.bstrVal != NULL)) {
    missing = (wcscmp(r.bstrVal, L"undefined") == 0);
  }
  VariantClear(&r);
  if (missing) {
    webview_mshtml_run(script, WEBVIEW_JS_LEGACY_RUNTIME, NULL);
  }
}

/* ---------------------------------------------------------------- F12 */

/* the document's script engine (Release it), or NULL */
static IDispatch *webview_mshtml_script(struct webview *w) {
  IWebBrowser2 *browser2 = NULL;
  IDispatch *document = NULL, *script = NULL;
  IHTMLDocument2 *html = NULL;
  IOleObject *browser;
  if ((w == NULL) || (w->priv.browser == NULL) || (*w->priv.browser == NULL)) {
    return NULL;
  }
  browser = *w->priv.browser;
  if (browser->lpVtbl->QueryInterface(browser, iid_unref(&IID_IWebBrowser2),
                                      (void **)&browser2) != S_OK) {
    return NULL;
  }
  if ((browser2->lpVtbl->get_Document(browser2, &document) == S_OK) &&
      (document != NULL)) {
    if (document->lpVtbl->QueryInterface(document,
                                         iid_unref(&IID_IHTMLDocument2),
                                         (void **)&html) == S_OK) {
      html->lpVtbl->get_Script(html, &script);
      html->lpVtbl->Release(html);
    }
    document->lpVtbl->Release(document);
  }
  browser2->lpVtbl->Release(browser2);
  return script;
}

/* F12 follows w.browser.devtools, and w.debug when that is left alone */
static int webview_ie_devtools_allowed(struct webview *w) {
  return (w->browser.devtools == WEBVIEW_ON) ||
         ((w->browser.devtools == WEBVIEW_DEFAULT) && w->debug);
}

/* the devtools drawn into the page (js/legacy_devtools.js): loaded the
   first time, then toggled (or just opened) */
static void webview_ie_devtools(struct webview *w, int toggle) {
  IDispatch *script = webview_mshtml_script(w);
  VARIANT r;
  int missing = 1;
  if (script == NULL) {
    return;
  }
  webview_mshtml_ensure_runtime(script);
  VariantInit(&r);
  if ((webview_mshtml_run(script, "typeof __wvdt", &r) == 0) &&
      (r.vt == VT_BSTR) && (r.bstrVal != NULL)) {
    missing = (wcscmp(r.bstrVal, L"undefined") == 0);
  }
  VariantClear(&r);
  if (missing) {
    webview_mshtml_run(script, WEBVIEW_JS_LEGACY_DEVTOOLS, NULL);
  }
  webview_mshtml_run(script, toggle ? "__wvdt.toggle()" : "__wvdt.open()",
                     NULL);
  script->lpVtbl->Release(script);
}

#endif /* _WIN32 */
