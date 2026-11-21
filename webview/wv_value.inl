#include <stdarg.h>

/* ---------------------------------------------------------------------------
 * wv_value.inl - implementation of wv_value.h
 *
 * Two halves:
 *   1. a small recursive descent JSON parser  (wv_parse ... wv_json)
 *   2. the bridge to the page                 (wv_eval ... wv_ref_call)
 * -------------------------------------------------------------------------*/

/* ==========================================================================
 * 1. JSON
 * ========================================================================*/

struct wv_value {
  wv_type type;
  double number;
  int boolean;
  char *string;      /* WV_STRING: the text. object members: reused for keys */
  wv_value **items;  /* WV_ARRAY / WV_OBJECT */
  char **keys;       /* WV_OBJECT */
  int count;
  int capacity;
};

static wv_value *wv_new(wv_type t) {
  wv_value *v = (wv_value *)calloc(1, sizeof(wv_value));
  if (v != NULL) {
    v->type = t;
  }
  return v;
}

static void wv_free(wv_value *v) {
  int i;
  if (v == NULL) {
    return;
  }
  for (i = 0; i < v->count; i++) {
    wv_free(v->items[i]);
    if (v->keys != NULL) {
      free(v->keys[i]);
    }
  }
  free(v->items);
  free(v->keys);
  free(v->string);
  free(v);
}

static int wv_push(wv_value *v, char *key, wv_value *child) {
  if (v->count == v->capacity) {
    int cap = (v->capacity == 0) ? 4 : v->capacity * 2;
    wv_value **items = (wv_value **)realloc(v->items, sizeof(*items) * cap);
    if (items == NULL) {
      return -1;
    }
    v->items = items;
    if (v->type == WV_OBJECT) {
      char **keys = (char **)realloc(v->keys, sizeof(*keys) * cap);
      if (keys == NULL) {
        return -1;
      }
      v->keys = keys;
    }
    v->capacity = cap;
  }
  v->items[v->count] = child;
  if (v->type == WV_OBJECT) {
    v->keys[v->count] = key;
  }
  v->count++;
  return 0;
}

static const char *wv_skip_ws(const char *p) {
  while ((*p == ' ') || (*p == '\t') || (*p == '\n') || (*p == '\r')) {
    p++;
  }
  return p;
}

static const char *wv_parse_value(const char *p, wv_value **out);

/* "...." -> malloc'ed UTF-8, with \u escapes folded into UTF-8 */
static const char *wv_parse_string(const char *p, char **out) {
  const char *start;
  char *s;
  char *q;
  if (*p != '"') {
    return NULL;
  }
  p++;
  start = p;
  while ((*p != '\0') && (*p != '"')) {
    if (*p == '\\') {
      p++;
      if (*p == '\0') {
        return NULL;
      }
    }
    p++;
  }
  if (*p != '"') {
    return NULL;
  }
  s = (char *)malloc((size_t)(p - start) * 3 + 1); /* \u -> max 3 bytes */
  if (s == NULL) {
    return NULL;
  }
  q = s;
  for (p = start; *p != '"';) {
    if (*p != '\\') {
      *q++ = *p++;
      continue;
    }
    p++;
    switch (*p) {
    case 'n': *q++ = '\n'; p++; break;
    case 't': *q++ = '\t'; p++; break;
    case 'r': *q++ = '\r'; p++; break;
    case 'b': *q++ = '\b'; p++; break;
    case 'f': *q++ = '\f'; p++; break;
    case '/': *q++ = '/';  p++; break;
    case '"': *q++ = '"';  p++; break;
    case '\\': *q++ = '\\'; p++; break;
    case 'u': {
      unsigned int cp = 0;
      int i;
      p++;
      for (i = 0; i < 4; i++) {
        char c = p[i];
        cp <<= 4;
        if ((c >= '0') && (c <= '9')) {
          cp |= (unsigned int)(c - '0');
        } else if ((c >= 'a') && (c <= 'f')) {
          cp |= (unsigned int)(c - 'a' + 10);
        } else if ((c >= 'A') && (c <= 'F')) {
          cp |= (unsigned int)(c - 'A' + 10);
        } else {
          free(s);
          return NULL;
        }
      }
      p += 4;
      /* Surrogate pairs are left as-is: WebView2 gives us well formed JSON,
         and folding them properly is more code than this example needs. */
      if (cp < 0x80) {
        *q++ = (char)cp;
      } else if (cp < 0x800) {
        *q++ = (char)(0xC0 | (cp >> 6));
        *q++ = (char)(0x80 | (cp & 0x3F));
      } else {
        *q++ = (char)(0xE0 | (cp >> 12));
        *q++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *q++ = (char)(0x80 | (cp & 0x3F));
      }
      break;
    }
    default:
      free(s);
      return NULL;
    }
  }
  *q = '\0';
  *out = s;
  return p + 1; /* past the closing quote */
}

static const char *wv_parse_value(const char *p, wv_value **out) {
  wv_value *v;
  p = wv_skip_ws(p);
  if (*p == '\0') {
    return NULL;
  }
  if (*p == '{') {
    v = wv_new(WV_OBJECT);
    if (v == NULL) {
      return NULL;
    }
    p = wv_skip_ws(p + 1);
    if (*p == '}') {
      *out = v;
      return p + 1;
    }
    for (;;) {
      char *key = NULL;
      wv_value *child = NULL;
      p = wv_parse_string(wv_skip_ws(p), &key);
      if (p == NULL) {
        wv_free(v);
        return NULL;
      }
      p = wv_skip_ws(p);
      if (*p != ':') {
        free(key);
        wv_free(v);
        return NULL;
      }
      p = wv_parse_value(p + 1, &child);
      if ((p == NULL) || (wv_push(v, key, child) != 0)) {
        free(key);
        wv_free(child);
        wv_free(v);
        return NULL;
      }
      p = wv_skip_ws(p);
      if (*p == ',') {
        p++;
        continue;
      }
      if (*p == '}') {
        *out = v;
        return p + 1;
      }
      wv_free(v);
      return NULL;
    }
  }
  if (*p == '[') {
    v = wv_new(WV_ARRAY);
    if (v == NULL) {
      return NULL;
    }
    p = wv_skip_ws(p + 1);
    if (*p == ']') {
      *out = v;
      return p + 1;
    }
    for (;;) {
      wv_value *child = NULL;
      p = wv_parse_value(p, &child);
      if ((p == NULL) || (wv_push(v, NULL, child) != 0)) {
        wv_free(child);
        wv_free(v);
        return NULL;
      }
      p = wv_skip_ws(p);
      if (*p == ',') {
        p++;
        continue;
      }
      if (*p == ']') {
        *out = v;
        return p + 1;
      }
      wv_free(v);
      return NULL;
    }
  }
  if (*p == '"') {
    char *s = NULL;
    p = wv_parse_string(p, &s);
    if (p == NULL) {
      return NULL;
    }
    v = wv_new(WV_STRING);
    if (v == NULL) {
      free(s);
      return NULL;
    }
    v->string = s;
    *out = v;
    return p;
  }
  if (strncmp(p, "true", 4) == 0) {
    v = wv_new(WV_BOOL);
    if (v == NULL) {
      return NULL;
    }
    v->boolean = 1;
    *out = v;
    return p + 4;
  }
  if (strncmp(p, "false", 5) == 0) {
    v = wv_new(WV_BOOL);
    if (v == NULL) {
      return NULL;
    }
    *out = v;
    return p + 5;
  }
  if (strncmp(p, "null", 4) == 0) {
    v = wv_new(WV_NULL);
    if (v == NULL) {
      return NULL;
    }
    *out = v;
    return p + 4;
  }
  {
    char *end = NULL;
    double d = strtod(p, &end);
    if ((end == NULL) || (end == p)) {
      return NULL;
    }
    v = wv_new(WV_NUMBER);
    if (v == NULL) {
      return NULL;
    }
    v->number = d;
    *out = v;
    return end;
  }
}

static wv_value *wv_parse(const char *json) {
  wv_value *v = NULL;
  const char *p;
  if (json == NULL) {
    return NULL;
  }
  p = wv_parse_value(json, &v);
  if (p == NULL) {
    return NULL;
  }
  return v;
}

/* --------------------------------------------------------------- accessors */

static wv_type wv_type_of(const wv_value *v) {
  return (v == NULL) ? WV_NULL : v->type;
}
static int wv_is_null(const wv_value *v) {
  return (v == NULL) || (v->type == WV_NULL);
}
static int wv_bool(const wv_value *v) {
  return ((v != NULL) && (v->type == WV_BOOL)) ? v->boolean : 0;
}
static double wv_number(const wv_value *v) {
  return ((v != NULL) && (v->type == WV_NUMBER)) ? v->number : 0.0;
}
static int wv_int(const wv_value *v) { return (int)wv_number(v); }
static const char *wv_string(const wv_value *v) {
  return ((v != NULL) && (v->type == WV_STRING)) ? v->string : NULL;
}
static int wv_count(const wv_value *v) { return (v == NULL) ? 0 : v->count; }

static wv_value *wv_at(const wv_value *v, int index) {
  if ((v == NULL) || (index < 0) || (index >= v->count)) {
    return NULL;
  }
  return v->items[index];
}

static const char *wv_key_at(const wv_value *v, int index) {
  if ((v == NULL) || (v->keys == NULL) || (index < 0) || (index >= v->count)) {
    return NULL;
  }
  return v->keys[index];
}

static wv_value *wv_get(const wv_value *v, const char *key) {
  int i;
  if ((v == NULL) || (v->type != WV_OBJECT) || (key == NULL)) {
    return NULL;
  }
  for (i = 0; i < v->count; i++) {
    if (strcmp(v->keys[i], key) == 0) {
      return v->items[i];
    }
  }
  return NULL;
}

/* ------------------------------------------------------------- re-encoding */

static int wv_json_append(char **buf, size_t *len, size_t *cap,
                          const char *text) {
  size_t n = strlen(text);
  if (*len + n + 1 > *cap) {
    size_t cap2 = (*cap == 0) ? 128 : *cap;
    char *b;
    while (*len + n + 1 > cap2) {
      cap2 *= 2;
    }
    b = (char *)realloc(*buf, cap2);
    if (b == NULL) {
      return -1;
    }
    *buf = b;
    *cap = cap2;
  }
  memcpy(*buf + *len, text, n + 1);
  *len += n;
  return 0;
}

static int wv_json_build(const wv_value *v, char **buf, size_t *len,
                         size_t *cap) {
  char scratch[64];
  int i;
  if (v == NULL) {
    return wv_json_append(buf, len, cap, "null");
  }
  switch (v->type) {
  case WV_NULL:
    return wv_json_append(buf, len, cap, "null");
  case WV_BOOL:
    return wv_json_append(buf, len, cap, v->boolean ? "true" : "false");
  case WV_NUMBER:
    snprintf(scratch, sizeof(scratch), "%.17g", v->number);
    return wv_json_append(buf, len, cap, scratch);
  case WV_STRING: {
    char *esc = webview_json_escape(v->string);
    int r;
    if (esc == NULL) {
      return -1;
    }
    r = wv_json_append(buf, len, cap, esc);
    free(esc);
    return r;
  }
  case WV_ARRAY:
  case WV_OBJECT:
    if (wv_json_append(buf, len, cap, (v->type == WV_ARRAY) ? "[" : "{") != 0) {
      return -1;
    }
    for (i = 0; i < v->count; i++) {
      if ((i > 0) && (wv_json_append(buf, len, cap, ",") != 0)) {
        return -1;
      }
      if (v->type == WV_OBJECT) {
        char *esc = webview_json_escape(v->keys[i]);
        int r;
        if (esc == NULL) {
          return -1;
        }
        r = wv_json_append(buf, len, cap, esc);
        free(esc);
        if ((r != 0) || (wv_json_append(buf, len, cap, ":") != 0)) {
          return -1;
        }
      }
      if (wv_json_build(v->items[i], buf, len, cap) != 0) {
        return -1;
      }
    }
    return wv_json_append(buf, len, cap, (v->type == WV_ARRAY) ? "]" : "}");
  }
  return -1;
}

static char *wv_json(const wv_value *v) {
  char *buf = NULL;
  size_t len = 0;
  size_t cap = 0;
  if (wv_json_build(v, &buf, &len, &cap) != 0) {
    free(buf);
    return NULL;
  }
  return buf;
}

/* ==========================================================================
 * 2. The bridge
 * ========================================================================*/

/* Heap allocated, not a stack local: if we give up waiting, the completion
   handler may still fire afterwards, and it must not write into a frame that
   has already returned. Whoever arrives last frees it. */
/* Every script below used to be built into a hand sized malloc + snprintf.
   Getting one of those constants wrong truncates the script into a syntax
   error that fails silently, which is a miserable bug to find - it happened
   twice in this codebase already. So: build the string, let it work out its
   own length, and never write another magic number. */
static char *wv_format(const char *fmt, ...) {
  size_t cap = 256;
  for (;;) {
    va_list ap;
    int n;
    char *buf = (char *)malloc(cap);
    if (buf == NULL) {
      return NULL;
    }
    va_start(ap, fmt);
    n = vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    if ((n >= 0) && ((size_t)n < cap)) {
      return buf; /* it fitted */
    }
    free(buf);
    if (cap > (16u << 20)) {
      return NULL; /* something is very wrong */
    }
    /* C99 returns the length it wanted; older runtimes just return -1 */
    cap = (n > 0) ? ((size_t)n + 1) : (cap * 2);
  }
}

typedef struct {
  int done;
  int abandoned;
  char *json;
} wv_sync_state;

static void wv_sync_cb(struct webview *w, const char *json, void *userdata) {
  wv_sync_state *state = (wv_sync_state *)userdata;
  size_t n;
  (void)w;
  if (state == NULL) {
    return;
  }
  if (state->abandoned) {
    free(state->json);
    free(state);
    return;
  }
  if (json != NULL) {
    n = strlen(json) + 1;
    state->json = (char *)malloc(n);
    if (state->json != NULL) {
      memcpy(state->json, json, n);
    }
  }
  state->done = 1;
}

/* Run `js` and wait for the answer, pumping messages meanwhile.
   Returns a malloc'ed JSON string, or NULL. */
static char *wv_eval_json(struct webview *w, const char *js, int timeout_ms) {
  wv_sync_state *state;
  DWORD started;
  MSG msg;
  char *json;
  if (timeout_ms <= 0) {
    timeout_ms = WV_TIMEOUT_DEFAULT;
  }
  state = (wv_sync_state *)calloc(1, sizeof(wv_sync_state));
  if (state == NULL) {
    return NULL;
  }
  if (webview_eval_result(w, js, &wv_sync_cb, state) != 0) {
    /* the IE fallback, or the webview has not been created yet */
    webview_print_log("wv_eval: the page cannot be evaluated (no WebView2, or "
                      "the window is not up yet)");
    free(state);
    return NULL;
  }
  started = GetTickCount();
  while (!state->done) {
    if (GetTickCount() - started > (DWORD)timeout_ms) {
      webview_print_log(
          "wv_eval: timed out. Are you calling this from inside "
          "external_invoke_cb? WebView2 will not deliver the answer while "
          "one of its own event handlers is still running - use "
          "webview_dispatch() to step out of it first.");
      state->abandoned = 1; /* the callback will free it when it arrives */
      return NULL;
    }
    if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        /* someone asked us to quit while we were waiting - put it back so
           webview_loop() sees it, and give up on this call */
        PostQuitMessage((int)msg.wParam);
        state->abandoned = 1;
        return NULL;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      Sleep(1);
    }
  }
  json = state->json;
  free(state);
  return json;
}

static wv_value *wv_eval(struct webview *w, const char *js, int timeout_ms) {
  char *json = wv_eval_json(w, js, timeout_ms);
  wv_value *v;
  if (json == NULL) {
    return NULL;
  }
  v = wv_parse(json);
  free(json);
  return v;
}

static wv_value *wv_call(struct webview *w, const char *fn,
                         const char *args_json, int timeout_ms) {
  char *js;
  wv_value *v;
  if ((w == NULL) || (fn == NULL)) {
    return NULL;
  }
  js = wv_format("(%s).apply(null,%s)", fn,
                 (args_json != NULL) ? args_json : "[]");
  if (js == NULL) {
    return NULL;
  }
  v = wv_eval(w, js, timeout_ms);
  free(js);
  return v;
}

/* --------------------------------------------------------------------------
 * Live handles
 *
 * The JS side table is created on first use:
 *   window.__wv_refs = { n: 1, map: {} }
 * A handle is just the integer key into that map.
 * ------------------------------------------------------------------------*/

#define WV_REFS_INIT                                                           \
  "(window.__wv_refs=window.__wv_refs||{n:0,map:{}})"

static wv_ref wv_ref_new(struct webview *w, const char *js, int timeout_ms) {
  char *code;
  wv_value *v;
  wv_ref ref = 0;
  if ((w == NULL) || (js == NULL)) {
    return 0;
  }
  code = wv_format("(function(){var t=%s;var v=(%s);"
                   "if(v===undefined||v===null)return 0;"
                   "t.n=t.n+1;t.map[t.n]=v;return t.n;})()",
                   WV_REFS_INIT, js);
  if (code == NULL) {
    return 0;
  }
  v = wv_eval(w, code, timeout_ms);
  free(code);
  if (v != NULL) {
    ref = wv_int(v);
    wv_free(v);
  }
  return ref;
}

static void wv_ref_free(struct webview *w, wv_ref ref) {
  char code[128];
  if ((w == NULL) || (ref <= 0)) {
    return;
  }
  snprintf(code, sizeof(code),
           "if(window.__wv_refs)delete window.__wv_refs.map[%d];", ref);
  webview_eval(w, code); /* fire and forget, no answer needed */
}

/* Build "window.__wv_refs.map[7]<path>" */
static char *wv_ref_expr(wv_ref ref, const char *path) {
  return wv_format("window.__wv_refs.map[%d]%s", ref,
                   (path != NULL) ? path : "");
}

static wv_value *wv_ref_get(struct webview *w, wv_ref ref, const char *path,
                            int timeout_ms) {
  char *expr;
  wv_value *v;
  if ((w == NULL) || (ref <= 0)) {
    return NULL;
  }
  expr = wv_ref_expr(ref, path);
  if (expr == NULL) {
    return NULL;
  }
  v = wv_eval(w, expr, timeout_ms);
  free(expr);
  return v;
}

static int wv_ref_set(struct webview *w, wv_ref ref, const char *path,
                      const char *json_value, int timeout_ms) {
  char *expr;
  char *code;
  wv_value *v;
  int ok;
  if ((w == NULL) || (ref <= 0) || (json_value == NULL)) {
    return -1;
  }
  expr = wv_ref_expr(ref, path);
  if (expr == NULL) {
    return -1;
  }
  code = wv_format("(%s=%s,true)", expr, json_value);
  if (code == NULL) {
    free(expr);
    return -1;
  }
  v = wv_eval(w, code, timeout_ms);
  free(code);
  free(expr);
  ok = wv_bool(v);
  wv_free(v);
  return ok ? 0 : -1;
}

static wv_value *wv_ref_call(struct webview *w, wv_ref ref, const char *method,
                             const char *args_json, int timeout_ms) {
  char *expr;
  char *code;
  wv_value *v;
  if ((w == NULL) || (ref <= 0)) {
    return NULL;
  }
  if (args_json == NULL) {
    args_json = "[]";
  }
  expr = wv_ref_expr(ref, NULL);
  if (expr == NULL) {
    return NULL;
  }
  if (method != NULL) {
    /* obj.method(...args) - `this` stays the object */
    code = wv_format("(%s).%s.apply(%s,%s)", expr, method, expr, args_json);
  } else {
    /* the ref is itself a function */
    code = wv_format("(%s).apply(null,%s)", expr, args_json);
  }
  if (code == NULL) {
    free(expr);
    return NULL;
  }
  v = wv_eval(w, code, timeout_ms);
  free(code);
  free(expr);
  return v;
}

static wv_ref wv_ref_child(struct webview *w, wv_ref ref, const char *path,
                           int timeout_ms) {
  char *expr;
  wv_ref child;
  if ((w == NULL) || (ref <= 0)) {
    return 0;
  }
  expr = wv_ref_expr(ref, path);
  if (expr == NULL) {
    return 0;
  }
  child = wv_ref_new(w, expr, timeout_ms);
  free(expr);
  return child;
}
