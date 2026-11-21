/* ---------------------------------------------------------------------------
 * wv_downlevel.inl - modern JavaScript, rewritten for old Internet Explorer
 *
 * The MSHTML backend runs on whatever Internet Explorer the system has:
 * IE6 on Windows 2000, IE8 on XP, IE9 on Vista, IE11 on 7. Their JScript
 * speaks ES3 (ES5 from IE9). This file parses modern JavaScript and writes
 * it back as ES3, at runtime, so a page written today runs there:
 *
 *   let / const          var, renamed where blocks would clash, and a
 *                        function per loop turn where a closure needs one
 *   arrow functions      function expressions; this / arguments captured
 *   template literals    string concatenation; tagged ones via __wv.tpl
 *   classes              constructor + prototype, extends, super, fields,
 *                        static members, getters and setters, #private
 *   destructuring        temporaries, in declarations, assignments, params
 *   default, rest        checks and slices at the top of the function
 *   spread               apply / concat / __wv.assign
 *   for...of             the iterator protocol, via __wv.it
 *   ?.  ??  **  ||= &&= ??=   conditionals, Math.pow, temporaries
 *   object literals      shorthand, methods, computed keys, spread
 *   literals             0b / 0o / 1_000 numbers, \u{...} escapes
 *   property names       reserved words quoted (ES3 refuses obj.default)
 *   trailing commas      removed (IE8 counts or refuses them)
 *
 * Not yet: async / await and generators (they need each function turned
 * into a state machine - the next stage), modules. Those fail with a
 * message; the caller then keeps the original script.
 *
 * The output calls helpers in the runtime that must run first in the page
 * (js/legacy_runtime.js): __wv.it, __wv.spread, __wv.assign, ...
 *
 * A script that uses nothing newer than ES3 comes back unchanged, so old
 * pages are never touched. Line numbers are kept: every statement starts on
 * the line it had, so IE's error messages point at the original source.
 *
 * Pure C, no Windows headers: it builds and is tested anywhere.
 *
 *   char err[256];
 *   char *es3 = webview_downlevel(src, strlen(src), err, sizeof(err));
 *   ... free(es3);   (NULL on failure, with the reason in err)
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WEBVIEW_API
#define WEBVIEW_API static
#endif

/* =============================================================== memory */

struct wvdl_chunk {
  struct wvdl_chunk *next;
  size_t used, size;
  /* data follows */
};

struct wvdl_arena {
  struct wvdl_chunk *head;
};

static void *wvdl_alloc(struct wvdl_arena *a, size_t n) {
  struct wvdl_chunk *c = a->head;
  n = (n + 7) & ~(size_t)7;
  if ((c == NULL) || (c->used + n > c->size)) {
    size_t size = (n > 65536) ? n : 65536;
    c = (struct wvdl_chunk *)malloc(sizeof(struct wvdl_chunk) + size);
    if (c == NULL) {
      return NULL;
    }
    c->next = a->head;
    c->used = 0;
    c->size = size;
    a->head = c;
  }
  {
    void *p = (char *)(c + 1) + c->used;
    c->used += n;
    memset(p, 0, n);
    return p;
  }
}

static void wvdl_arena_free(struct wvdl_arena *a) {
  while (a->head != NULL) {
    struct wvdl_chunk *next = a->head->next;
    free(a->head);
    a->head = next;
  }
}

/* a growing string, kept in the arena (so an error frees it too) */
struct wvdl_buf {
  char *p;
  size_t len, cap;
  int failed;
  struct wvdl_arena *arena;
};

static void wvdl_put(struct wvdl_buf *b, const char *s, size_t n) {
  if (b->failed) {
    return;
  }
  if (b->len + n + 1 > b->cap) {
    size_t cap = b->cap ? b->cap * 2 : 256;
    char *p;
    while (cap < b->len + n + 1) {
      cap *= 2;
    }
    p = (char *)wvdl_alloc(b->arena, cap);
    if (p == NULL) {
      b->failed = 1;
      return;
    }
    if (b->len > 0) {
      memcpy(p, b->p, b->len);
    }
    b->p = p;
    b->cap = cap;
  }
  memcpy(b->p + b->len, s, n);
  b->len += n;
  b->p[b->len] = 0;
}

static void wvdl_puts(struct wvdl_buf *b, const char *s) {
  wvdl_put(b, s, strlen(s));
}

/* ================================================================ tokens */

enum {
  WVDL_T_EOF,
  WVDL_T_NAME,    /* identifier or keyword */
  WVDL_T_PRIVATE, /* #name */
  WVDL_T_NUM,
  WVDL_T_STR,
  WVDL_T_TPL, /* a template chunk: `...${  or  }...${  or  }...` */
  WVDL_T_REGEX,
  WVDL_T_PUNCT
};

/* punctuators, longest first within each start character */
static const char *const wvdl_puncts[] = {
    ">>>=", "...", "===", "!==", "**=", "<<=", ">>=", ">>>", "&&=", "||=",
    "?\?=", "=>", "==", "!=", "<=", ">=", "&&", "||", "??", "?.", "++", "--",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>", "**", "{",
    "}", "(", ")", "[", "]", ";", ",", "<", ">", "+", "-", "*", "/", "%",
    "&", "|", "^", "!", "~", "?", ":", "=", ".", "@", NULL};

struct wvdl_tok {
  int type;
  int start, end; /* byte offsets in the source */
  int nl;         /* a line break before it */
  int tail;       /* T_TPL: ends with ` rather than ${ */
};

/* ============================================================ the tree */

enum {
  /* expressions */
  N_ID = 1, N_NUM, N_STR, N_REGEX, N_TEMPLATE, N_TAGGED, N_THIS, N_SUPER,
  N_LIT,  /* null true false */
  N_ARRAY, N_OBJECT, N_PROP, N_FUNC, N_CLASS, N_MEMBER_DEF,
  N_UNARY, N_UPDATE, N_BINARY, N_ASSIGN, N_COND, N_CALL, N_NEW, N_MEMBER,
  N_SEQ, N_SPREAD, N_YIELD, N_AWAIT, N_CHAIN, N_HOLE, N_PRIVATE_NAME,
  N_NEW_TARGET,
  /* patterns */
  N_ARRAY_PAT, N_OBJECT_PAT, N_ASSIGN_PAT, N_REST,
  /* statements */
  N_PROGRAM, N_BLOCK, N_VAR, N_DECL, N_EXPR_STMT, N_IF, N_FOR, N_FOR_IN,
  N_FOR_OF, N_WHILE, N_DO, N_RETURN, N_BREAK, N_CONTINUE, N_THROW, N_TRY,
  N_SWITCH, N_CASE, N_LABEL, N_EMPTY, N_DEBUGGER,
  /* only made by the lowering */
  N_RAW /* text printed as is (helpers' names, temporaries) */
};

/* N_FUNC flags */
#define WVDL_F_ARROW 1
#define WVDL_F_ASYNC 2
#define WVDL_F_GEN 4
#define WVDL_F_EXPR_BODY 8 /* arrow with an expression for a body */
#define WVDL_F_DECL 16     /* a declaration, not an expression */
#define WVDL_F_METHOD 32
/* N_MEMBER / N_CALL flags */
#define WVDL_F_COMPUTED 1
#define WVDL_F_OPTIONAL 2
/* N_PROP / N_MEMBER_DEF kinds (op) */
enum { P_INIT, P_SHORT, P_METHOD, P_GET, P_SET, P_SPREAD, P_FIELD,
       P_CTOR, P_STATIC_BLOCK };
/* N_VAR kinds (op) */
enum { K_VAR, K_LET, K_CONST };

struct wvdl_scope;
struct wvdl_binding;
struct wvdl_name;
struct wvdl_fn;
struct wvdl_cls;
struct wvdl_jump;

typedef struct wvdl_node wvdl_node;
struct wvdl_node {
  int type;
  int op;    /* operator (punct index / keyword code), or kind */
  int flags;
  int pos;   /* source offset, for messages */
  const char *str; /* names, raw literal text */
  int len;
  wvdl_node *a, *b, *c, *d;
  wvdl_node *list; /* first child of a list */
  wvdl_node *next; /* next sibling in a list */
  int is_static;
  int paren; /* written in parentheses: never a destructuring target */
  int extra; /* WVDL_X_* marks of the lowering */
  struct wvdl_scope *scope;     /* scopes that this node opens */
  struct wvdl_binding *binding; /* N_ID after resolution */
};

/* ============================================================== context */

struct wvdl {
  const char *src;
  int len;
  int pos; /* lexer position */
  struct wvdl_tok tok;
  int prev_end; /* end of the token before tok */
  struct wvdl_arena arena;
  jmp_buf fail;
  char *err;
  size_t errlen;
  /* parser state */
  int in_function, in_async, in_generator, in_class;
  int modern; /* uses something newer than ES3 */
  /* scope analysis */
  struct wvdl_name *names; /* hash table: bindings, declared, free names */
  unsigned names_cap, names_used;
  struct wvdl_binding *blocked;  /* block-scoped bindings */
  struct wvdl_binding *bindings; /* all of them */
  /* printer */
  int counter;        /* for fresh names */
  const char *prefix; /* "$wv" unless the source uses it */
  char tpl_id[16];    /* tells this script's template sites apart */
  int tpl_sites;
  int cls_counter;
  struct wvdl_buf *out;
  int line; /* output line of the end of the output */
  int *line_starts;
  int line_count;
  int no_in; /* parenthesize `in` (inside a for head) */
  struct wvdl_fn *fn;
  struct wvdl_cls *cls;
  struct wvdl_jump *jumps;
};

/* ================================================= IE8 and older: names */

/*
 * IE8 and older have no getters or setters on plain objects, and IE6/7 no
 * Element.prototype to give elements what they lack. With the old IE mode
 * on (webview_downlevel_mode), access to a few names goes through the
 * runtime, which knows where the getter, the setter or the method is:
 *
 *   o.k  -> __wv.g(o, "k")          o.k = v  -> __wv.s(o, "k", v)
 *   o.k += v -> __wv.s(o, "k", __wv.g(o, "k") + (v))
 *   o.k++    -> __wv.u(o, "k", 1, 0)
 *   o.m(...) -> __wv.d(o).m(...)    an IE6/7 element gets the method first
 *
 * The names: every getter and setter any script declared so far (scripts
 * load in order, so a class comes before the code that uses it), and the
 * element properties and methods IE6/7 lack. o[name] with a computed name
 * and destructuring are not rewritten.
 */
#ifndef WEBVIEW_DOWNLEVEL_OLD_IE
#define WEBVIEW_DOWNLEVEL_OLD_IE 1
#endif /* !WEBVIEW_DOWNLEVEL_OLD_IE */

static int wvdl_mode_flags = 0;
static char **wvdl_acc_v = NULL;
static int wvdl_acc_n = 0, wvdl_acc_cap = 0;

#ifdef _WIN32
static volatile LONG wvdl_acc_busy = 0;
#define WVDL_ACC_LOCK() \
  while (InterlockedExchange(&wvdl_acc_busy, 1) != 0) { Sleep(0); }
#define WVDL_ACC_UNLOCK() InterlockedExchange(&wvdl_acc_busy, 0)
#else /* !_WIN32 */
#define WVDL_ACC_LOCK()
#define WVDL_ACC_UNLOCK()
#endif /* _WIN32 */

/* what IE6/7 elements lack: read through __wv.g, written through __wv.s */
static const char *const wvdl_dom_props[] = {
    "textContent", "classList", "firstElementChild", "lastElementChild",
    "nextElementSibling", "previousElementSibling", "childElementCount",
    NULL};
/* ... and called through __wv.d(o) */
static const char *const wvdl_dom_methods[] = {
    "addEventListener", "removeEventListener", "dispatchEvent",
    "querySelector", "querySelectorAll", "getElementsByClassName",
    "matches", "closest", "remove", "hasAttribute", "append", "prepend",
    "before", "after", "replaceWith", NULL};

static int wvdl_word_in(const char *s, int len, const char *const *list) {
  int i;
  for (i = 0; list[i] != NULL; i++) {
    if (((int)strlen(list[i]) == len) && (memcmp(list[i], s, len) == 0)) {
      return 1;
    }
  }
  return 0;
}

static int wvdl_acc_known(const char *s, int len) {
  int i, found = 0;
  WVDL_ACC_LOCK();
  for (i = 0; (i < wvdl_acc_n) && !found; i++) {
    found = ((int)strlen(wvdl_acc_v[i]) == len) &&
            (memcmp(wvdl_acc_v[i], s, len) == 0);
  }
  WVDL_ACC_UNLOCK();
  return found;
}

/* a getter or a setter called s: from now on o.s goes through __wv.g/s */
static void wvdl_acc_add(const char *s, int len) {
  char *copy;
  if ((len <= 0) || wvdl_acc_known(s, len)) {
    return;
  }
  copy = (char *)malloc((size_t)len + 1);
  if (copy == NULL) {
    return;
  }
  memcpy(copy, s, len);
  copy[len] = 0;
  WVDL_ACC_LOCK();
  if (wvdl_acc_n == wvdl_acc_cap) {
    int cap = wvdl_acc_cap ? wvdl_acc_cap * 2 : 32;
    char **v = (char **)realloc(wvdl_acc_v, sizeof(char *) * cap);
    if (v == NULL) {
      WVDL_ACC_UNLOCK();
      free(copy);
      return;
    }
    wvdl_acc_v = v;
    wvdl_acc_cap = cap;
  }
  wvdl_acc_v[wvdl_acc_n++] = copy;
  WVDL_ACC_UNLOCK();
}

/* the name of a key node: an identifier, or a string without escapes */
static int wvdl_key_name(wvdl_node *k, const char **s, int *len) {
  if (k == NULL) {
    return 0;
  }
  if (k->type == N_ID) {
    *s = k->str;
    *len = k->len;
    return 1;
  }
  if ((k->type == N_STR) && (k->str != NULL) && (k->len >= 2) &&
      (memchr(k->str, '\\', k->len) == NULL)) {
    *s = k->str + 1;
    *len = k->len - 2;
    return 1;
  }
  return 0;
}

/* 0: plain; 1: read and write through __wv.g/s; 2: call through __wv.d */
static int wvdl_name_kind(const char *s, int len) {
  if (!(wvdl_mode_flags & WEBVIEW_DOWNLEVEL_OLD_IE)) {
    return 0;
  }
  if (wvdl_word_in(s, len, wvdl_dom_methods)) {
    return 2;
  }
  if (wvdl_word_in(s, len, wvdl_dom_props) || wvdl_acc_known(s, len)) {
    return 1;
  }
  return 0;
}

static int wvdl_member_kind(wvdl_node *m) {
  const char *s;
  int len;
  if ((m == NULL) || (m->type != N_MEMBER) || !wvdl_mode_flags ||
      (m->a->type == N_SUPER)) {
    return 0;
  }
  if (!(m->flags & WVDL_F_COMPUTED) && (m->b->type != N_ID)) {
    return 0; /* #private */
  }
  if (!wvdl_key_name(m->b, &s, &len)) {
    return 0;
  }
  return wvdl_name_kind(s, len);
}

/* the parser saw a getter or setter: remember its name for every script */
static void wvdl_note_accessor(wvdl_node *key, int computed) {
  const char *s;
  int len;
  if ((wvdl_mode_flags & WEBVIEW_DOWNLEVEL_OLD_IE) && !computed &&
      wvdl_key_name(key, &s, &len)) {
    wvdl_acc_add(s, len);
  }
}

static void wvdl_line_col(struct wvdl *x, int pos, int *line, int *col) {
  int i;
  *line = 1;
  *col = 1;
  for (i = 0; (i < pos) && (i < x->len); i++) {
    if (x->src[i] == '\n') {
      (*line)++;
      *col = 1;
    } else {
      (*col)++;
    }
  }
}

static void wvdl_error(struct wvdl *x, int pos, const char *fmt, ...) {
  int line, col, n = 0;
  va_list ap;
  wvdl_line_col(x, pos, &line, &col);
  if ((x->err != NULL) && (x->errlen > 0)) {
    n = snprintf(x->err, x->errlen, "%d:%d: ", line, col);
    if ((n >= 0) && ((size_t)n < x->errlen)) {
      va_start(ap, fmt);
      vsnprintf(x->err + n, x->errlen - (size_t)n, fmt, ap);
      va_end(ap);
    }
  }
  longjmp(x->fail, 1);
}

static wvdl_node *wvdl_new(struct wvdl *x, int type, int pos) {
  wvdl_node *n = (wvdl_node *)wvdl_alloc(&x->arena, sizeof(wvdl_node));
  if (n == NULL) {
    wvdl_error(x, pos, "out of memory");
  }
  switch (type) {
  case N_TEMPLATE: case N_TAGGED: case N_CLASS: case N_SPREAD: case N_CHAIN:
  case N_ASSIGN_PAT: case N_REST: case N_YIELD: case N_AWAIT:
  case N_NEW_TARGET: case N_FOR_OF: case N_PRIVATE_NAME: case N_SUPER:
    x->modern = 1;
    break;
  default:
    break;
  }
  n->type = type;
  n->pos = pos;
  return n;
}

static char *wvdl_strdup(struct wvdl *x, const char *s, int n) {
  char *p = (char *)wvdl_alloc(&x->arena, (size_t)n + 1);
  if (p == NULL) {
    wvdl_error(x, 0, "out of memory");
  }
  memcpy(p, s, (size_t)n);
  p[n] = 0;
  return p;
}

/* ================================================================ lexer */

static int wvdl_id_start(unsigned char c) {
  return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
         (c == '_') || (c == '$') || (c >= 0x80) || (c == '\\');
}

static int wvdl_id_part(unsigned char c) {
  return wvdl_id_start(c) || ((c >= '0') && (c <= '9'));
}

/* line terminators: \n \r, and U+2028 / U+2029 (E2 80 A8/A9) */
static int wvdl_newline_at(struct wvdl *x, int i) {
  unsigned char c = (unsigned char)x->src[i];
  if ((c == '\n') || (c == '\r')) {
    return 1;
  }
  if ((c == 0xE2) && (i + 2 < x->len) &&
      ((unsigned char)x->src[i + 1] == 0x80) &&
      (((unsigned char)x->src[i + 2] == 0xA8) ||
       ((unsigned char)x->src[i + 2] == 0xA9))) {
    return 3;
  }
  return 0;
}

/* skip white space and comments; returns whether a line break was crossed */
static int wvdl_skip_space(struct wvdl *x) {
  int nl = 0;
  while (x->pos < x->len) {
    unsigned char c = (unsigned char)x->src[x->pos];
    int n;
    if ((n = wvdl_newline_at(x, x->pos)) != 0) {
      nl = 1;
      x->pos += n;
    } else if ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f')) {
      x->pos++;
    } else if ((c == 0xC2) && (x->pos + 1 < x->len) &&
               ((unsigned char)x->src[x->pos + 1] == 0xA0)) {
      x->pos += 2; /* no-break space */
    } else if ((c == 0xEF) && (x->pos + 2 < x->len) &&
               ((unsigned char)x->src[x->pos + 1] == 0xBB) &&
               ((unsigned char)x->src[x->pos + 2] == 0xBF)) {
      x->pos += 3; /* byte order mark */
    } else if ((c == '/') && (x->pos + 1 < x->len) &&
               (x->src[x->pos + 1] == '/')) {
      while ((x->pos < x->len) && !wvdl_newline_at(x, x->pos)) {
        x->pos++;
      }
    } else if ((c == '/') && (x->pos + 1 < x->len) &&
               (x->src[x->pos + 1] == '*')) {
      int start = x->pos;
      x->pos += 2;
      while ((x->pos + 1 < x->len) &&
             !((x->src[x->pos] == '*') && (x->src[x->pos + 1] == '/'))) {
        if (wvdl_newline_at(x, x->pos)) {
          nl = 1;
        }
        x->pos++;
      }
      if (x->pos + 1 >= x->len) {
        wvdl_error(x, start, "unterminated comment");
      }
      x->pos += 2;
    } else if ((c == '<') && (x->pos + 3 < x->len) &&
               (strncmp(x->src + x->pos, "<!--", 4) == 0)) {
      /* HTML-like comment, still met in old inline scripts */
      while ((x->pos < x->len) && !wvdl_newline_at(x, x->pos)) {
        x->pos++;
      }
    } else if (nl && (c == '-') && (x->pos + 2 < x->len) &&
               (strncmp(x->src + x->pos, "-->", 3) == 0)) {
      while ((x->pos < x->len) && !wvdl_newline_at(x, x->pos)) {
        x->pos++;
      }
    } else {
      break;
    }
  }
  return nl;
}

/* a template chunk from x->pos (just after ` or }) to ` or ${ */
static void wvdl_scan_template(struct wvdl *x, int start) {
  while (x->pos < x->len) {
    char c = x->src[x->pos];
    if (c == '\\') {
      x->pos += 2;
      continue;
    }
    if (c == '`') {
      x->pos++;
      x->tok.type = WVDL_T_TPL;
      x->tok.tail = 1;
      x->tok.start = start;
      x->tok.end = x->pos;
      return;
    }
    if ((c == '$') && (x->pos + 1 < x->len) && (x->src[x->pos + 1] == '{')) {
      x->pos += 2;
      x->tok.type = WVDL_T_TPL;
      x->tok.tail = 0;
      x->tok.start = start;
      x->tok.end = x->pos;
      return;
    }
    x->pos++;
  }
  wvdl_error(x, start, "unterminated template literal");
}

static void wvdl_next(struct wvdl *x) {
  int start;
  unsigned char c;
  int i;
  x->prev_end = x->tok.end;
  x->tok.nl = wvdl_skip_space(x);
  x->tok.tail = 0;
  start = x->pos;
  x->tok.start = start;
  if (x->pos >= x->len) {
    x->tok.type = WVDL_T_EOF;
    x->tok.end = x->pos;
    return;
  }
  c = (unsigned char)x->src[x->pos];
  if (wvdl_id_start(c) || ((c == '#') && (x->pos + 1 < x->len) &&
                           wvdl_id_start((unsigned char)x->src[x->pos + 1]))) {
    x->tok.type = (c == '#') ? WVDL_T_PRIVATE : WVDL_T_NAME;
    x->pos++;
    while ((x->pos < x->len) &&
           wvdl_id_part((unsigned char)x->src[x->pos])) {
      if (x->src[x->pos] == '\\') {
        x->pos += 2; /* \uXXXX: the hex digits are id parts */
      } else {
        x->pos++;
      }
    }
    x->tok.end = x->pos;
    return;
  }
  if (((c >= '0') && (c <= '9')) ||
      ((c == '.') && (x->pos + 1 < x->len) && (x->src[x->pos + 1] >= '0') &&
       (x->src[x->pos + 1] <= '9'))) {
    x->tok.type = WVDL_T_NUM;
    if ((c == '0') && (x->pos + 1 < x->len) && (x->src[x->pos + 1] != 0) &&
        strchr("xXoObB", x->src[x->pos + 1])) {
      x->pos += 2;
      while ((x->pos < x->len) && (isxdigit((unsigned char)x->src[x->pos]) ||
                                   (x->src[x->pos] == '_'))) {
        x->pos++;
      }
    } else {
      while ((x->pos < x->len) && (isdigit((unsigned char)x->src[x->pos]) ||
                                   (x->src[x->pos] == '_'))) {
        x->pos++;
      }
      if ((x->pos < x->len) && (x->src[x->pos] == '.')) {
        x->pos++;
        while ((x->pos < x->len) && (isdigit((unsigned char)x->src[x->pos]) ||
                                     (x->src[x->pos] == '_'))) {
          x->pos++;
        }
      }
      if ((x->pos < x->len) &&
          ((x->src[x->pos] == 'e') || (x->src[x->pos] == 'E'))) {
        x->pos++;
        if ((x->pos < x->len) &&
            ((x->src[x->pos] == '+') || (x->src[x->pos] == '-'))) {
          x->pos++;
        }
        while ((x->pos < x->len) && isdigit((unsigned char)x->src[x->pos])) {
          x->pos++;
        }
      }
    }
    if ((x->pos < x->len) && (x->src[x->pos] == 'n')) {
      x->pos++; /* BigInt: refused later, with a message */
    }
    x->tok.end = x->pos;
    return;
  }
  if ((c == '"') || (c == '\'')) {
    x->tok.type = WVDL_T_STR;
    x->pos++;
    while ((x->pos < x->len) && (x->src[x->pos] != (char)c)) {
      if (x->src[x->pos] == '\\') {
        x->pos++;
      } else if ((x->src[x->pos] == '\n') || (x->src[x->pos] == '\r')) {
        wvdl_error(x, start, "unterminated string");
      }
      x->pos++;
    }
    if (x->pos >= x->len) {
      wvdl_error(x, start, "unterminated string");
    }
    x->pos++;
    x->tok.end = x->pos;
    return;
  }
  if (c == '`') {
    x->pos++;
    wvdl_scan_template(x, start);
    return;
  }
  for (i = 0; wvdl_puncts[i] != NULL; i++) {
    size_t n = strlen(wvdl_puncts[i]);
    if ((x->pos + (int)n <= x->len) &&
        (memcmp(x->src + x->pos, wvdl_puncts[i], n) == 0)) {
      /* `?.` followed by a digit is `?` then a number: a ? .5 : 1 */
      if ((n == 2) && (wvdl_puncts[i][0] == '?') && (wvdl_puncts[i][1] == '.') &&
          (x->pos + 2 < x->len) && (x->src[x->pos + 2] >= '0') &&
          (x->src[x->pos + 2] <= '9')) {
        continue;
      }
      x->tok.type = WVDL_T_PUNCT;
      x->pos += (int)n;
      x->tok.end = x->pos;
      return;
    }
  }
  wvdl_error(x, start, "unexpected character '%c'", c);
}

/* the parser met / or /= where an expression starts: it is a regex */
static void wvdl_rescan_regex(struct wvdl *x) {
  int in_class = 0;
  x->pos = x->tok.start + 1;
  while (1) {
    char c;
    if ((x->pos >= x->len) || wvdl_newline_at(x, x->pos)) {
      wvdl_error(x, x->tok.start, "unterminated regular expression");
    }
    c = x->src[x->pos];
    if (c == '\\') {
      x->pos += 2;
      continue;
    }
    if (c == '[') {
      in_class = 1;
    } else if (c == ']') {
      in_class = 0;
    } else if ((c == '/') && !in_class) {
      break;
    }
    x->pos++;
  }
  x->pos++;
  while ((x->pos < x->len) && wvdl_id_part((unsigned char)x->src[x->pos])) {
    x->pos++;
  }
  x->tok.type = WVDL_T_REGEX;
  x->tok.end = x->pos;
}

/* the parser finished ${ expr }: the template goes on after the } */
static void wvdl_rescan_template(struct wvdl *x) {
  x->pos = x->tok.start + 1;
  wvdl_scan_template(x, x->tok.start);
}

/* ------------------------------------------------------- token helpers */

static int wvdl_is(struct wvdl *x, const char *p) {
  size_t n;
  if (x->tok.type != WVDL_T_PUNCT) {
    return 0;
  }
  n = strlen(p);
  return ((size_t)(x->tok.end - x->tok.start) == n) &&
         (memcmp(x->src + x->tok.start, p, n) == 0);
}

static int wvdl_is_name(struct wvdl *x, const char *s) {
  size_t n;
  if (x->tok.type != WVDL_T_NAME) {
    return 0;
  }
  n = strlen(s);
  return ((size_t)(x->tok.end - x->tok.start) == n) &&
         (memcmp(x->src + x->tok.start, s, n) == 0);
}

static int wvdl_eat(struct wvdl *x, const char *p) {
  if (wvdl_is(x, p)) {
    wvdl_next(x);
    return 1;
  }
  return 0;
}

static void wvdl_expect(struct wvdl *x, const char *p) {
  if (!wvdl_eat(x, p)) {
    wvdl_error(x, x->tok.start, "expected '%s'", p);
  }
}

static int wvdl_punct_id(const char *s, int n) {
  int i;
  for (i = 0; wvdl_puncts[i] != NULL; i++) {
    if (((int)strlen(wvdl_puncts[i]) == n) &&
        (memcmp(wvdl_puncts[i], s, (size_t)n) == 0)) {
      return i;
    }
  }
  return -1;
}

static int wvdl_tok_op(struct wvdl *x) {
  return wvdl_punct_id(x->src + x->tok.start, x->tok.end - x->tok.start);
}

static const char *wvdl_op_text(int op) {
  return wvdl_puncts[op];
}

static int wvdl_op_is(int op, const char *s) {
  return (op >= 0) && (strcmp(wvdl_puncts[op], s) == 0);
}

static const char *const wvdl_reserved[] = {
    "break", "case", "catch", "class", "const", "continue", "debugger",
    "default", "delete", "do", "else", "enum", "export", "extends", "false",
    "finally", "for", "function", "if", "import", "in", "instanceof", "new",
    "null", "return", "super", "switch", "this", "throw", "true", "try",
    "typeof", "var", "void", "while", "with", NULL};

/* ES3 also kept these back for the future: not usable as obj.name */
static const char *const wvdl_es3_reserved[] = {
    "abstract", "boolean", "byte", "char", "double", "final", "float", "goto",
    "implements", "int", "interface", "long", "native", "package", "private",
    "protected", "public", "short", "static", "synchronized", "throws",
    "transient", "volatile", "let", "yield", "await", NULL};

/* the ones of them an old engine could refuse as a variable name */
static const char *const wvdl_future_words[] = {
    "abstract", "boolean", "byte", "char", "double", "final", "float", "goto",
    "implements", "int", "interface", "long", "native", "package", "private",
    "protected", "public", "short", "static", "synchronized", "throws",
    "transient", "volatile", NULL};

static int wvdl_in_list(const char *const *list, const char *s, int n) {
  int i;
  for (i = 0; list[i] != NULL; i++) {
    if (((int)strlen(list[i]) == n) && (memcmp(list[i], s, (size_t)n) == 0)) {
      return 1;
    }
  }
  return 0;
}

static int wvdl_tok_reserved(struct wvdl *x) {
  return (x->tok.type == WVDL_T_NAME) &&
         wvdl_in_list(wvdl_reserved, x->src + x->tok.start,
                      x->tok.end - x->tok.start);
}

/* lexer state, to look ahead and come back */
struct wvdl_mark {
  int pos, prev_end;
  struct wvdl_tok tok;
};

static void wvdl_save(struct wvdl *x, struct wvdl_mark *m) {
  m->pos = x->pos;
  m->prev_end = x->prev_end;
  m->tok = x->tok;
}

static void wvdl_restore(struct wvdl *x, struct wvdl_mark *m) {
  x->pos = m->pos;
  x->prev_end = m->prev_end;
  x->tok = m->tok;
}

/* automatic semicolon insertion */
static void wvdl_semicolon(struct wvdl *x) {
  if (wvdl_eat(x, ";")) {
    return;
  }
  if (wvdl_is(x, "}") || (x->tok.type == WVDL_T_EOF) || x->tok.nl) {
    return;
  }
  wvdl_error(x, x->tok.start, "expected ';'");
}

/* ============================================================== literals */

static void wvdl_put_char(struct wvdl_buf *b, char c) {
  wvdl_put(b, &c, 1);
}

static void wvdl_put_u16(struct wvdl_buf *b, unsigned u) {
  char t[8];
  snprintf(t, sizeof(t), "\\u%04X", u & 0xFFFF);
  wvdl_puts(b, t);
}

static void wvdl_put_code_point(struct wvdl_buf *b, unsigned long cp) {
  if (cp >= 0x10000) {
    cp -= 0x10000;
    wvdl_put_u16(b, (unsigned)(0xD800 + (cp >> 10)));
    wvdl_put_u16(b, (unsigned)(0xDC00 + (cp & 0x3FF)));
  } else {
    wvdl_put_u16(b, (unsigned)cp);
  }
}

/* one UTF-8 sequence at s (s[0] >= 0x80) as \uXXXX; returns its length */
static int wvdl_put_utf8(struct wvdl_buf *b, const char *s, int n) {
  const unsigned char *u = (const unsigned char *)s;
  unsigned long cp = u[0];
  int len = 1, i;
  if ((u[0] & 0xE0) == 0xC0) {
    len = 2;
    cp = u[0] & 0x1F;
  } else if ((u[0] & 0xF0) == 0xE0) {
    len = 3;
    cp = u[0] & 0x0F;
  } else if ((u[0] & 0xF8) == 0xF0) {
    len = 4;
    cp = u[0] & 0x07;
  }
  if (len > n) {
    len = 1;
  }
  for (i = 1; i < len; i++) {
    if ((u[i] & 0xC0) != 0x80) {
      len = 1;
      cp = u[0];
      break;
    }
    cp = (cp << 6) | (u[i] & 0x3F);
  }
  wvdl_put_code_point(b, cp);
  return len;
}

static int wvdl_hex_value(char c) {
  if ((c >= '0') && (c <= '9')) {
    return c - '0';
  }
  if ((c >= 'a') && (c <= 'f')) {
    return c - 'a' + 10;
  }
  if ((c >= 'A') && (c <= 'F')) {
    return c - 'A' + 10;
  }
  return -1;
}

static int wvdl_hex_run(const char *s, int n, int count) {
  int i;
  if (n < count) {
    return 0;
  }
  for (i = 0; i < count; i++) {
    if (wvdl_hex_value(s[i]) < 0) {
      return 0;
    }
  }
  return 1;
}

/* A string or template text made safe for ES3, between double quotes,
   pure ASCII. `quote` is the delimiter of the original (' " or `).
   NULL when an escape is broken (a tagged template then gets undefined). */
static char *wvdl_es3_string(struct wvdl *x, const char *s, int n, char quote) {
  struct wvdl_buf b;
  int i;
  memset(&b, 0, sizeof(b));
  b.arena = &x->arena;
  wvdl_put_char(&b, '"');
  for (i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '\\') {
      char e = (i + 1 < n) ? s[i + 1] : 0;
      if (e == 'u') {
        if ((i + 2 < n) && (s[i + 2] == '{')) {
          unsigned long cp = 0;
          int j = i + 3;
          while ((j < n) && (wvdl_hex_value(s[j]) >= 0)) {
            cp = cp * 16 + (unsigned long)wvdl_hex_value(s[j]);
            j++;
          }
          if ((j >= n) || (s[j] != '}') || (j == i + 3) || (cp > 0x10FFFF)) {
            return NULL;
          }
          wvdl_put_code_point(&b, cp);
          x->modern = 1;
          i = j;
          continue;
        }
        if (!wvdl_hex_run(s + i + 2, n - i - 2, 4)) {
          return NULL;
        }
      } else if ((e == 'x') && !wvdl_hex_run(s + i + 2, n - i - 2, 2)) {
        return NULL;
      }
      if ((e == '\r') || (e == '\n')) { /* line continuation */
        i++;
        if ((e == '\r') && (i + 1 < n) && (s[i + 1] == '\n')) {
          i++;
        }
        continue;
      }
      if ((e == '`') || (e == '$') || (e == '\'')) {
        wvdl_put_char(&b, e); /* no escape needed between " */
        i++;
        continue;
      }
      if ((unsigned char)e >= 0x80) { /* backslash, non-ASCII: the char */
        i += wvdl_put_utf8(&b, s + i + 1, n - i - 1);
        continue;
      }
      wvdl_put_char(&b, '\\');
      wvdl_put_char(&b, e);
      i++;
      continue;
    }
    if (c == '"') {
      wvdl_puts(&b, "\\\"");
    } else if (c == '\n') {
      wvdl_puts(&b, "\\n");
    } else if (c == '\r') {
      if ((i + 1 < n) && (s[i + 1] == '\n')) {
        i++; /* a template's CRLF is LF */
      }
      wvdl_puts(&b, (quote == '`') ? "\\n" : "\\r");
    } else if (c == '\t') {
      wvdl_puts(&b, "\\t");
    } else if (c >= 0x80) {
      if ((c == 0xE2) && (i + 2 < n) && ((unsigned char)s[i + 1] == 0x80) &&
          (((unsigned char)s[i + 2] == 0xA8) ||
           ((unsigned char)s[i + 2] == 0xA9))) {
        x->modern = 1; /* a line break inside a string: ES2019 */
      }
      i += wvdl_put_utf8(&b, s + i, n - i) - 1;
    } else if (c < 0x20) {
      wvdl_put_u16(&b, c);
    } else {
      wvdl_put_char(&b, (char)c);
    }
  }
  wvdl_put_char(&b, '"');
  if (b.failed) {
    wvdl_error(x, 0, "out of memory");
  }
  return b.p;
}

/* the raw text of a template chunk, as the string literal of String.raw */
static char *wvdl_raw_string(struct wvdl *x, const char *s, int n) {
  struct wvdl_buf b;
  int i;
  memset(&b, 0, sizeof(b));
  b.arena = &x->arena;
  wvdl_put_char(&b, '"');
  for (i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '\\') {
      wvdl_puts(&b, "\\\\");
    } else if (c == '"') {
      wvdl_puts(&b, "\\\"");
    } else if (c == '\n') {
      wvdl_puts(&b, "\\n");
    } else if (c == '\r') {
      if ((i + 1 < n) && (s[i + 1] == '\n')) {
        i++;
      }
      wvdl_puts(&b, "\\n");
    } else if (c == '\t') {
      wvdl_puts(&b, "\\t");
    } else if (c >= 0x80) {
      i += wvdl_put_utf8(&b, s + i, n - i) - 1;
    } else {
      wvdl_put_char(&b, (char)c);
    }
  }
  wvdl_put_char(&b, '"');
  if (b.failed) {
    wvdl_error(x, 0, "out of memory");
  }
  return b.p;
}

/* 0b1010, 0o17, 1_000_000 -> plain decimal; BigInt refused */
static char *wvdl_es3_number(struct wvdl *x, int start, int end) {
  const char *s = x->src + start;
  int n = end - start, i, j = 0;
  char clean[128];
  if (n >= (int)sizeof(clean)) {
    return wvdl_strdup(x, s, n);
  }
  for (i = 0; i < n; i++) {
    if (s[i] != '_') {
      clean[j++] = s[i];
    }
  }
  clean[j] = 0;
  if (j != n) {
    x->modern = 1;
  }
  if ((j > 0) && (clean[j - 1] == 'n')) {
    wvdl_error(x, start, "BigInt literals cannot run on this engine");
  }
  if ((j > 2) && (clean[0] == '0') && strchr("oObB", clean[1])) {
    double v = 0;
    int base = ((clean[1] == 'b') || (clean[1] == 'B')) ? 2 : 8;
    char out[64];
    for (i = 2; i < j; i++) {
      v = v * base + (clean[i] - '0');
    }
    snprintf(out, sizeof(out), "%.0f", v);
    x->modern = 1;
    return wvdl_strdup(x, out, (int)strlen(out));
  }
  return wvdl_strdup(x, clean, j);
}

static char *wvdl_string_token(struct wvdl *x) {
  char *s = wvdl_es3_string(x, x->src + x->tok.start + 1,
                            x->tok.end - x->tok.start - 2,
                            x->src[x->tok.start]);
  if (s == NULL) {
    wvdl_error(x, x->tok.start, "invalid escape in string");
  }
  return s;
}

/* ================================================================ parser */

static wvdl_node *wvdl_expr(struct wvdl *x, int no_in);
static wvdl_node *wvdl_assign(struct wvdl *x, int no_in);
static wvdl_node *wvdl_statement(struct wvdl *x);
static wvdl_node *wvdl_function(struct wvdl *x, int flags, int pos);
static wvdl_node *wvdl_class(struct wvdl *x, int is_decl);
static wvdl_node *wvdl_binding_target(struct wvdl *x);
static void wvdl_to_pattern(struct wvdl *x, wvdl_node *n, int binding);

/* append to a list kept as (head, tail) */
static void wvdl_append(wvdl_node **head, wvdl_node **tail, wvdl_node *n) {
  if (*tail == NULL) {
    *head = n;
  } else {
    (*tail)->next = n;
  }
  *tail = n;
}

static wvdl_node *wvdl_ident(struct wvdl *x) {
  wvdl_node *n;
  if (x->tok.type != WVDL_T_NAME) {
    wvdl_error(x, x->tok.start, "expected a name");
  }
  n = wvdl_new(x, N_ID, x->tok.start);
  n->str = x->src + x->tok.start;
  n->len = x->tok.end - x->tok.start;
  wvdl_next(x);
  return n;
}

/* a property name: identifier (any word), string, number, [computed], #x */
static wvdl_node *wvdl_prop_key(struct wvdl *x, int *computed) {
  wvdl_node *k;
  *computed = 0;
  if (wvdl_eat(x, "[")) {
    k = wvdl_assign(x, 0);
    wvdl_expect(x, "]");
    *computed = 1;
    x->modern = 1;
    return k;
  }
  if (x->tok.type == WVDL_T_STR) {
    k = wvdl_new(x, N_STR, x->tok.start);
    k->str = wvdl_string_token(x);
    k->len = (int)strlen(k->str);
    wvdl_next(x);
    return k;
  }
  if (x->tok.type == WVDL_T_NUM) {
    k = wvdl_new(x, N_NUM, x->tok.start);
    k->str = wvdl_es3_number(x, x->tok.start, x->tok.end);
    k->len = (int)strlen(k->str);
    wvdl_next(x);
    return k;
  }
  if (x->tok.type == WVDL_T_PRIVATE) {
    k = wvdl_new(x, N_PRIVATE_NAME, x->tok.start);
    k->str = x->src + x->tok.start + 1;
    k->len = x->tok.end - x->tok.start - 1;
    wvdl_next(x);
    return k;
  }
  if (x->tok.type == WVDL_T_NAME) {
    if (wvdl_tok_reserved(x)) {
      x->modern = 1; /* { default: 1 }: ES5 */
    }
    k = wvdl_new(x, N_ID, x->tok.start);
    k->str = x->src + x->tok.start;
    k->len = x->tok.end - x->tok.start;
    wvdl_next(x);
    return k;
  }
  wvdl_error(x, x->tok.start, "expected a property name");
  return NULL;
}

static wvdl_node *wvdl_template(struct wvdl *x) {
  wvdl_node *t = wvdl_new(x, N_TEMPLATE, x->tok.start);
  wvdl_node *head = NULL, *tail = NULL;
  while (1) {
    /* the chunk's text, without ` or } in front and ` or ${ behind */
    int s = x->tok.start + 1;
    int e = x->tok.end - (x->tok.tail ? 1 : 2);
    int tail_chunk = x->tok.tail;
    wvdl_node *q = wvdl_new(x, N_STR, s);
    q->str = wvdl_es3_string(x, x->src + s, e - s, '`');
    q->len = (q->str != NULL) ? (int)strlen(q->str) : 0;
    q->a = wvdl_new(x, N_STR, s); /* the raw text, for tagged templates */
    q->a->str = wvdl_raw_string(x, x->src + s, e - s);
    q->a->len = (int)strlen(q->a->str);
    wvdl_append(&head, &tail, q);
    if (tail_chunk) {
      wvdl_next(x);
      break;
    }
    wvdl_next(x);
    wvdl_append(&head, &tail, wvdl_expr(x, 0));
    if (!wvdl_is(x, "}")) {
      wvdl_error(x, x->tok.start, "expected '}' in template literal");
    }
    wvdl_rescan_template(x);
  }
  t->list = head;
  return t;
}

static wvdl_node *wvdl_array_literal(struct wvdl *x) {
  wvdl_node *n = wvdl_new(x, N_ARRAY, x->tok.start);
  wvdl_node *head = NULL, *tail = NULL;
  wvdl_expect(x, "[");
  while (!wvdl_is(x, "]")) {
    if (wvdl_is(x, ",")) {
      wvdl_append(&head, &tail, wvdl_new(x, N_HOLE, x->tok.start));
      wvdl_next(x);
      if (wvdl_is(x, "]")) {
        x->modern = 1; /* IE8 counts a trailing comma */
      }
      continue;
    }
    if (wvdl_is(x, "...")) {
      wvdl_node *s = wvdl_new(x, N_SPREAD, x->tok.start);
      wvdl_next(x);
      s->a = wvdl_assign(x, 0);
      wvdl_append(&head, &tail, s);
    } else {
      wvdl_append(&head, &tail, wvdl_assign(x, 0));
    }
    if (!wvdl_is(x, "]")) {
      wvdl_expect(x, ",");
      if (wvdl_is(x, "]")) {
        x->modern = 1;
      }
    }
  }
  wvdl_next(x);
  n->list = head;
  return n;
}

/* a method's parameters and body; `kind` is P_METHOD / P_GET / P_SET ... */
static wvdl_node *wvdl_method(struct wvdl *x, int flags, int pos) {
  return wvdl_function(x, flags | WVDL_F_METHOD, pos);
}

static wvdl_node *wvdl_object_literal(struct wvdl *x) {
  wvdl_node *n = wvdl_new(x, N_OBJECT, x->tok.start);
  wvdl_node *head = NULL, *tail = NULL;
  wvdl_expect(x, "{");
  while (!wvdl_is(x, "}")) {
    wvdl_node *p = wvdl_new(x, N_PROP, x->tok.start);
    int computed = 0, flags = 0;
    if (wvdl_is(x, "...")) {
      wvdl_next(x);
      p->op = P_SPREAD;
      p->b = wvdl_assign(x, 0);
      wvdl_append(&head, &tail, p);
      if (!wvdl_is(x, "}")) {
        wvdl_expect(x, ",");
      }
      continue;
    }
    if (wvdl_is_name(x, "async") || wvdl_is_name(x, "get") ||
        wvdl_is_name(x, "set")) {
      struct wvdl_mark m;
      int word = wvdl_is_name(x, "async") ? 1 : wvdl_is_name(x, "get") ? 2 : 3;
      wvdl_save(x, &m);
      wvdl_next(x);
      if (!wvdl_is(x, ",") && !wvdl_is(x, ":") && !wvdl_is(x, "(") &&
          !wvdl_is(x, "}") && !wvdl_is(x, "=") && !x->tok.nl) {
        if (word == 1) {
          flags |= WVDL_F_ASYNC;
          p->op = P_METHOD;
        } else {
          p->op = (word == 2) ? P_GET : P_SET;
        }
      } else {
        wvdl_restore(x, &m);
      }
    }
    if (wvdl_eat(x, "*")) {
      flags |= WVDL_F_GEN;
      p->op = P_METHOD;
    }
    p->a = wvdl_prop_key(x, &computed);
    p->flags = computed ? WVDL_F_COMPUTED : 0;
    if ((p->op == P_GET) || (p->op == P_SET)) {
      wvdl_note_accessor(p->a, computed);
    }
    if ((p->op == P_GET) || (p->op == P_SET) || (p->op == P_METHOD) ||
        wvdl_is(x, "(")) {
      if (p->op == P_INIT) {
        p->op = P_METHOD;
      }
      p->b = wvdl_method(x, flags, p->pos);
    } else if (wvdl_eat(x, ":")) {
      p->op = P_INIT;
      p->b = wvdl_assign(x, 0);
    } else if (p->a->type == N_ID) {
      /* shorthand, or a pattern default: { a = 1 } = obj */
      p->op = P_SHORT;
      p->b = wvdl_new(x, N_ID, p->a->pos);
      p->b->str = p->a->str;
      p->b->len = p->a->len;
      if (wvdl_is(x, "=")) {
        wvdl_node *d = wvdl_new(x, N_ASSIGN_PAT, x->tok.start);
        wvdl_next(x);
        d->a = p->b;
        d->b = wvdl_assign(x, 0);
        p->b = d; /* only valid once this becomes a pattern */
      }
    } else {
      wvdl_error(x, x->tok.start, "expected ':'");
    }
    if (p->op != P_INIT) {
      x->modern = 1; /* shorthand, methods, getters and setters */
    }
    wvdl_append(&head, &tail, p);
    if (!wvdl_is(x, "}")) {
      wvdl_expect(x, ",");
      if (wvdl_is(x, "}")) {
        x->modern = 1;
      }
    }
  }
  wvdl_next(x);
  n->list = head;
  return n;
}

/* parameters: ( a, b = 1, {c}, ...rest ) */
static wvdl_node *wvdl_params(struct wvdl *x) {
  wvdl_node *head = NULL, *tail = NULL;
  wvdl_expect(x, "(");
  while (!wvdl_is(x, ")")) {
    wvdl_node *p;
    if (wvdl_is(x, "...")) {
      p = wvdl_new(x, N_REST, x->tok.start);
      wvdl_next(x);
      p->a = wvdl_binding_target(x);
    } else {
      p = wvdl_binding_target(x);
      if (wvdl_is(x, "=")) {
        wvdl_node *d = wvdl_new(x, N_ASSIGN_PAT, x->tok.start);
        wvdl_next(x);
        d->a = p;
        d->b = wvdl_assign(x, 0);
        p = d;
      }
    }
    wvdl_append(&head, &tail, p);
    if (!wvdl_is(x, ")")) {
      wvdl_expect(x, ",");
      if (wvdl_is(x, ")")) {
        x->modern = 1;
      }
    }
  }
  wvdl_next(x);
  return head;
}

static wvdl_node *wvdl_block_body(struct wvdl *x) {
  wvdl_node *b = wvdl_new(x, N_BLOCK, x->tok.start);
  wvdl_node *head = NULL, *tail = NULL;
  wvdl_expect(x, "{");
  while (!wvdl_is(x, "}")) {
    if (x->tok.type == WVDL_T_EOF) {
      wvdl_error(x, b->pos, "unterminated block");
    }
    wvdl_append(&head, &tail, wvdl_statement(x));
  }
  wvdl_next(x);
  b->list = head;
  return b;
}

/* after `function` (and its name), or a method's key */
static wvdl_node *wvdl_function(struct wvdl *x, int flags, int pos) {
  wvdl_node *f = wvdl_new(x, N_FUNC, pos);
  int saved_f = x->in_function, saved_a = x->in_async, saved_g = x->in_generator;
  f->flags = flags;
  if (flags & (WVDL_F_ASYNC | WVDL_F_GEN)) {
    x->modern = 1;
  }
  x->in_function = 1;
  x->in_async = (flags & WVDL_F_ASYNC) != 0;
  x->in_generator = (flags & WVDL_F_GEN) != 0;
  f->list = wvdl_params(x);
  f->b = wvdl_block_body(x);
  x->in_function = saved_f;
  x->in_async = saved_a;
  x->in_generator = saved_g;
  return f;
}

/* the body of an arrow, after => */
static wvdl_node *wvdl_arrow_body(struct wvdl *x, wvdl_node *f, int no_in) {
  int saved_f = x->in_function, saved_a = x->in_async, saved_g = x->in_generator;
  x->modern = 1;
  x->in_function = 1;
  x->in_async = (f->flags & WVDL_F_ASYNC) != 0;
  x->in_generator = 0;
  if (wvdl_is(x, "{")) {
    f->b = wvdl_block_body(x);
  } else {
    f->flags |= WVDL_F_EXPR_BODY;
    f->b = wvdl_assign(x, no_in);
  }
  x->in_function = saved_f;
  x->in_async = saved_a;
  x->in_generator = saved_g;
  return f;
}

/* (a, b) or a, already parsed as an expression, turned into parameters */
static wvdl_node *wvdl_params_from(struct wvdl *x, wvdl_node *e) {
  wvdl_node *head = NULL, *tail = NULL, *p, *next;
  if (e == NULL) {
    return NULL;
  }
  p = (e->type == N_SEQ) ? e->list : e;
  for (; p != NULL; p = next) {
    next = (e->type == N_SEQ) ? p->next : NULL;
    p->next = NULL;
    if (p->type == N_SPREAD) {
      p->type = N_REST;
      wvdl_to_pattern(x, p->a, 1);
    } else {
      wvdl_to_pattern(x, p, 1);
    }
    wvdl_append(&head, &tail, p);
  }
  return head;
}

static wvdl_node *wvdl_paren_or_arrow(struct wvdl *x, int no_in, int async,
                                      int pos) {
  /* ( ... ) : an expression, or an arrow's parameters */
  wvdl_node *inner = NULL, *head = NULL, *tail = NULL;
  int has_rest = 0, trailing = 0;
  wvdl_expect(x, "(");
  while (!wvdl_is(x, ")")) {
    wvdl_node *e;
    trailing = 0;
    if (wvdl_is(x, "...")) {
      e = wvdl_new(x, N_SPREAD, x->tok.start);
      wvdl_next(x);
      e->a = wvdl_binding_target(x);
      has_rest = 1;
    } else {
      e = wvdl_assign(x, 0);
    }
    wvdl_append(&head, &tail, e);
    if (!wvdl_is(x, ")")) {
      wvdl_expect(x, ",");
      trailing = 1;
    }
  }
  wvdl_next(x);
  if (wvdl_is(x, "=>") && !x->tok.nl) {
    wvdl_node *f = wvdl_new(x, N_FUNC, pos);
    wvdl_node *seq = NULL;
    wvdl_next(x);
    f->flags = WVDL_F_ARROW | (async ? WVDL_F_ASYNC : 0);
    if (head != NULL) {
      seq = wvdl_new(x, N_SEQ, head->pos);
      seq->list = head;
    }
    f->list = wvdl_params_from(x, seq);
    return wvdl_arrow_body(x, f, no_in);
  }
  if ((head == NULL) || has_rest || trailing) {
    wvdl_error(x, pos, "expected '=>'");
  }
  if (async) {
    /* async(...) was a call to something named async */
    wvdl_node *c = wvdl_new(x, N_CALL, pos);
    c->a = wvdl_new(x, N_ID, pos);
    c->a->str = "async";
    c->a->len = 5;
    c->list = head;
    return c;
  }
  if (head->next == NULL) {
    inner = head;
  } else {
    inner = wvdl_new(x, N_SEQ, head->pos);
    inner->list = head;
  }
  inner->paren = 1; /* never a target of a destructuring */
  return inner;
}

static wvdl_node *wvdl_primary(struct wvdl *x, int no_in) {
  wvdl_node *n;
  int pos = x->tok.start;
  switch (x->tok.type) {
  case WVDL_T_NUM:
    n = wvdl_new(x, N_NUM, pos);
    n->str = wvdl_es3_number(x, x->tok.start, x->tok.end);
    n->len = (int)strlen(n->str);
    wvdl_next(x);
    return n;
  case WVDL_T_STR:
    n = wvdl_new(x, N_STR, pos);
    n->str = wvdl_string_token(x);
    n->len = (int)strlen(n->str);
    wvdl_next(x);
    return n;
  case WVDL_T_TPL:
    return wvdl_template(x);
  case WVDL_T_PRIVATE:
    /* #x in obj */
    n = wvdl_new(x, N_PRIVATE_NAME, pos);
    n->str = x->src + x->tok.start + 1;
    n->len = x->tok.end - x->tok.start - 1;
    wvdl_next(x);
    return n;
  case WVDL_T_PUNCT:
    if (wvdl_is(x, "(")) {
      return wvdl_paren_or_arrow(x, no_in, 0, pos);
    }
    if (wvdl_is(x, "[")) {
      return wvdl_array_literal(x);
    }
    if (wvdl_is(x, "{")) {
      return wvdl_object_literal(x);
    }
    if (wvdl_is(x, "/") || wvdl_is(x, "/=")) {
      wvdl_rescan_regex(x);
      n = wvdl_new(x, N_REGEX, pos);
      n->str = x->src + x->tok.start;
      n->len = x->tok.end - x->tok.start;
      {
        int i, end = n->len - 1;
        while (n->str[end] != '/') {
          if (!strchr("gim", n->str[end])) {
            x->modern = 1; /* s u y d flags */
          }
          end--;
        }
        for (i = 1; i + 3 < end; i++) {
          if (n->str[i] == '\\') {
            i++;
          } else if ((n->str[i] == '(') && (n->str[i + 1] == '?') &&
                     (n->str[i + 2] == '<') && (n->str[i + 3] != '=') &&
                     (n->str[i + 3] != '!')) {
            x->modern = 1; /* named groups */
          }
        }
      }
      wvdl_next(x);
      return n;
    }
    break;
  case WVDL_T_NAME:
    if (wvdl_is_name(x, "function")) {
      int flags = 0;
      wvdl_next(x);
      if (wvdl_eat(x, "*")) {
        flags |= WVDL_F_GEN;
      }
      n = NULL;
      if (x->tok.type == WVDL_T_NAME) {
        n = wvdl_ident(x);
      }
      {
        wvdl_node *f = wvdl_function(x, flags, pos);
        f->a = n;
        return f;
      }
    }
    if (wvdl_is_name(x, "async")) {
      struct wvdl_mark m;
      wvdl_save(x, &m);
      wvdl_next(x);
      if (wvdl_is_name(x, "function") && !x->tok.nl) {
        int flags = WVDL_F_ASYNC;
        wvdl_node *name = NULL, *f;
        wvdl_next(x);
        if (wvdl_eat(x, "*")) {
          flags |= WVDL_F_GEN;
        }
        if (x->tok.type == WVDL_T_NAME) {
          name = wvdl_ident(x);
        }
        f = wvdl_function(x, flags, pos);
        f->a = name;
        return f;
      }
      if ((x->tok.type == WVDL_T_NAME) && !x->tok.nl && !wvdl_tok_reserved(x)) {
        /* async x => ... */
        wvdl_node *p = wvdl_ident(x);
        if (wvdl_is(x, "=>") && !x->tok.nl) {
          wvdl_node *f = wvdl_new(x, N_FUNC, pos);
          wvdl_next(x);
          f->flags = WVDL_F_ARROW | WVDL_F_ASYNC;
          f->list = p;
          return wvdl_arrow_body(x, f, no_in);
        }
        wvdl_restore(x, &m);
      } else if (wvdl_is(x, "(") && !x->tok.nl) {
        return wvdl_paren_or_arrow(x, no_in, 1, pos);
      } else {
        wvdl_restore(x, &m);
      }
    }
    if (wvdl_is_name(x, "class")) {
      return wvdl_class(x, 0);
    }
    if (wvdl_is_name(x, "this")) {
      wvdl_next(x);
      return wvdl_new(x, N_THIS, pos);
    }
    if (wvdl_is_name(x, "super")) {
      wvdl_next(x);
      return wvdl_new(x, N_SUPER, pos);
    }
    if (wvdl_is_name(x, "null") || wvdl_is_name(x, "true") ||
        wvdl_is_name(x, "false")) {
      n = wvdl_new(x, N_LIT, pos);
      n->str = x->src + x->tok.start;
      n->len = x->tok.end - x->tok.start;
      wvdl_next(x);
      return n;
    }
    if (wvdl_is_name(x, "new")) {
      break; /* handled by the caller */
    }
    if (wvdl_tok_reserved(x)) {
      break;
    }
    n = wvdl_ident(x);
    if (wvdl_is(x, "=>") && !x->tok.nl) {
      wvdl_node *f = wvdl_new(x, N_FUNC, pos);
      wvdl_next(x);
      f->flags = WVDL_F_ARROW;
      f->list = n;
      return wvdl_arrow_body(x, f, no_in);
    }
    return n;
  default:
    break;
  }
  wvdl_error(x, x->tok.start, "unexpected %s",
             (x->tok.type == WVDL_T_EOF) ? "end of script" : "token");
  return NULL;
}

/* arguments: ( a, ...b ) */
static wvdl_node *wvdl_arguments(struct wvdl *x) {
  wvdl_node *head = NULL, *tail = NULL;
  wvdl_expect(x, "(");
  while (!wvdl_is(x, ")")) {
    if (wvdl_is(x, "...")) {
      wvdl_node *s = wvdl_new(x, N_SPREAD, x->tok.start);
      wvdl_next(x);
      s->a = wvdl_assign(x, 0);
      wvdl_append(&head, &tail, s);
    } else {
      wvdl_append(&head, &tail, wvdl_assign(x, 0));
    }
    if (!wvdl_is(x, ")")) {
      wvdl_expect(x, ",");
      if (wvdl_is(x, ")")) {
        x->modern = 1;
      }
    }
  }
  wvdl_next(x);
  return head;
}

/* member access, calls, tagged templates, optional chains */
static wvdl_node *wvdl_postfix_chain(struct wvdl *x, wvdl_node *n, int no_call) {
  int in_chain = 0;
  while (1) {
    int pos = x->tok.start;
    if (wvdl_is(x, ".")) {
      wvdl_node *m = wvdl_new(x, N_MEMBER, pos);
      wvdl_next(x);
      m->a = n;
      if (x->tok.type == WVDL_T_PRIVATE) {
        m->b = wvdl_new(x, N_PRIVATE_NAME, x->tok.start);
        m->b->str = x->src + x->tok.start + 1;
        m->b->len = x->tok.end - x->tok.start - 1;
        wvdl_next(x);
      } else if (x->tok.type == WVDL_T_NAME) {
        if (wvdl_tok_reserved(x)) {
          x->modern = 1; /* obj.default: ES5 */
        }
        m->b = wvdl_new(x, N_ID, x->tok.start);
        m->b->str = x->src + x->tok.start;
        m->b->len = x->tok.end - x->tok.start;
        wvdl_next(x);
      } else {
        wvdl_error(x, x->tok.start, "expected a property name");
      }
      if (wvdl_member_kind(m)) {
        x->modern = 1; /* goes through the runtime on old IE */
      }
      n = m;
    } else if (wvdl_is(x, "?.")) {
      in_chain = 1;
      wvdl_next(x);
      if (wvdl_is(x, "(")) {
        wvdl_node *c = wvdl_new(x, N_CALL, pos);
        c->a = n;
        c->flags = WVDL_F_OPTIONAL;
        c->list = wvdl_arguments(x);
        n = c;
      } else if (wvdl_eat(x, "[")) {
        wvdl_node *m = wvdl_new(x, N_MEMBER, pos);
        m->a = n;
        m->b = wvdl_expr(x, 0);
        m->flags = WVDL_F_COMPUTED | WVDL_F_OPTIONAL;
        wvdl_expect(x, "]");
        n = m;
      } else {
        wvdl_node *m = wvdl_new(x, N_MEMBER, pos);
        m->a = n;
        m->flags = WVDL_F_OPTIONAL;
        if (x->tok.type == WVDL_T_PRIVATE) {
          m->b = wvdl_new(x, N_PRIVATE_NAME, x->tok.start);
          m->b->str = x->src + x->tok.start + 1;
          m->b->len = x->tok.end - x->tok.start - 1;
          wvdl_next(x);
        } else {
          m->b = wvdl_new(x, N_ID, x->tok.start);
          if (x->tok.type != WVDL_T_NAME) {
            wvdl_error(x, x->tok.start, "expected a property name");
          }
          m->b->str = x->src + x->tok.start;
          m->b->len = x->tok.end - x->tok.start;
          wvdl_next(x);
        }
        n = m;
      }
    } else if (wvdl_eat(x, "[")) {
      wvdl_node *m = wvdl_new(x, N_MEMBER, pos);
      m->a = n;
      m->b = wvdl_expr(x, 0);
      m->flags = WVDL_F_COMPUTED;
      wvdl_expect(x, "]");
      if (wvdl_member_kind(m)) {
        x->modern = 1;
      }
      n = m;
    } else if (!no_call && wvdl_is(x, "(")) {
      wvdl_node *c = wvdl_new(x, N_CALL, pos);
      c->a = n;
      c->list = wvdl_arguments(x);
      n = c;
    } else if (x->tok.type == WVDL_T_TPL) {
      wvdl_node *t = wvdl_new(x, N_TAGGED, pos);
      t->a = n;
      t->b = wvdl_template(x);
      n = t;
    } else {
      break;
    }
  }
  if (in_chain) {
    wvdl_node *c = wvdl_new(x, N_CHAIN, n->pos);
    c->a = n;
    n = c;
  }
  return n;
}

static wvdl_node *wvdl_new_expr(struct wvdl *x, int no_in) {
  int pos = x->tok.start;
  if (wvdl_is_name(x, "new")) {
    wvdl_node *n;
    wvdl_next(x);
    if (wvdl_eat(x, ".")) {
      if (!wvdl_is_name(x, "target")) {
        wvdl_error(x, x->tok.start, "expected new.target");
      }
      wvdl_next(x);
      return wvdl_new(x, N_NEW_TARGET, pos);
    }
    n = wvdl_new(x, N_NEW, pos);
    n->a = wvdl_new_expr(x, no_in);
    n->a = wvdl_postfix_chain(x, n->a, 1);
    if (wvdl_is(x, "(")) {
      n->list = wvdl_arguments(x);
    }
    return n;
  }
  return wvdl_primary(x, no_in);
}

static wvdl_node *wvdl_lhs(struct wvdl *x, int no_in) {
  wvdl_node *n = wvdl_new_expr(x, no_in);
  if ((n->type == N_FUNC) && (n->flags & WVDL_F_ARROW) && !n->paren) {
    return n; /* an arrow is not followed by .x or (...) */
  }
  return wvdl_postfix_chain(x, n, 0);
}

static wvdl_node *wvdl_unary(struct wvdl *x, int no_in) {
  int pos = x->tok.start;
  if (wvdl_is(x, "!") || wvdl_is(x, "~") || wvdl_is(x, "+") ||
      wvdl_is(x, "-") || wvdl_is_name(x, "typeof") ||
      wvdl_is_name(x, "void") || wvdl_is_name(x, "delete")) {
    wvdl_node *n = wvdl_new(x, N_UNARY, pos);
    if (x->tok.type == WVDL_T_PUNCT) {
      n->op = wvdl_tok_op(x);
      n->str = NULL;
    } else {
      n->op = -1;
      n->str = x->src + x->tok.start;
      n->len = x->tok.end - x->tok.start;
    }
    wvdl_next(x);
    n->a = wvdl_unary(x, no_in);
    return n;
  }
  if (wvdl_is(x, "++") || wvdl_is(x, "--")) {
    wvdl_node *n = wvdl_new(x, N_UPDATE, pos);
    n->op = wvdl_tok_op(x);
    n->flags = 1; /* prefix */
    wvdl_next(x);
    n->a = wvdl_unary(x, no_in);
    return n;
  }
  if (wvdl_is_name(x, "await") && x->in_async) {
    wvdl_node *n = wvdl_new(x, N_AWAIT, pos);
    wvdl_next(x);
    n->a = wvdl_unary(x, no_in);
    return n;
  }
  {
    wvdl_node *n = wvdl_lhs(x, no_in);
    if ((wvdl_is(x, "++") || wvdl_is(x, "--")) && !x->tok.nl) {
      wvdl_node *u = wvdl_new(x, N_UPDATE, x->tok.start);
      u->op = wvdl_tok_op(x);
      u->a = n;
      wvdl_next(x);
      return u;
    }
    return n;
  }
}

static int wvdl_binary_prec(struct wvdl *x, int no_in, int *op_out) {
  static const struct {
    const char *op;
    int prec;
  } table[] = {{"??", 1},  {"||", 2},  {"&&", 3},   {"|", 4},    {"^", 5},
               {"&", 6},   {"==", 7},  {"!=", 7},   {"===", 7},  {"!==", 7},
               {"<", 8},   {">", 8},   {"<=", 8},   {">=", 8},   {"<<", 9},
               {">>", 9},  {">>>", 9}, {"+", 10},   {"-", 10},   {"*", 11},
               {"/", 11},  {"%", 11},  {"**", 12},  {NULL, 0}};
  int i;
  if (x->tok.type == WVDL_T_NAME) {
    if (wvdl_is_name(x, "instanceof") || (!no_in && wvdl_is_name(x, "in"))) {
      *op_out = -1;
      return 8;
    }
    return 0;
  }
  if (x->tok.type != WVDL_T_PUNCT) {
    return 0;
  }
  for (i = 0; table[i].op != NULL; i++) {
    if (wvdl_is(x, table[i].op)) {
      *op_out = wvdl_tok_op(x);
      return table[i].prec;
    }
  }
  return 0;
}

static wvdl_node *wvdl_binary(struct wvdl *x, int min_prec, int no_in) {
  wvdl_node *left = wvdl_unary(x, no_in);
  while (1) {
    int op = 0, prec = wvdl_binary_prec(x, no_in, &op);
    wvdl_node *n;
    if ((prec == 0) || (prec < min_prec)) {
      break;
    }
    n = wvdl_new(x, N_BINARY, x->tok.start);
    n->op = op;
    if ((prec == 1) || (prec == 12)) {
      x->modern = 1; /* ?? and ** */
    }
    if (op < 0) {
      n->str = x->src + x->tok.start;
      n->len = x->tok.end - x->tok.start;
    }
    wvdl_next(x);
    n->a = left;
    /* ** is right-associative */
    n->b = wvdl_binary(x, (prec == 12) ? prec : prec + 1, no_in);
    left = n;
  }
  return left;
}

static wvdl_node *wvdl_conditional(struct wvdl *x, int no_in) {
  wvdl_node *test = wvdl_binary(x, 1, no_in);
  if (wvdl_is(x, "?")) {
    wvdl_node *n = wvdl_new(x, N_COND, x->tok.start);
    wvdl_next(x);
    n->a = test;
    n->b = wvdl_assign(x, 0);
    wvdl_expect(x, ":");
    n->c = wvdl_assign(x, no_in);
    return n;
  }
  return test;
}

/* turn an expression parsed before = or => into a pattern */
static void wvdl_to_pattern(struct wvdl *x, wvdl_node *n, int binding) {
  wvdl_node *p;
  if ((n->type != N_ID) && (n->type != N_MEMBER)) {
    x->modern = 1;
  }
  switch (n->type) {
  case N_ID:
    return;
  case N_MEMBER:
    if (binding) {
      wvdl_error(x, n->pos, "invalid binding");
    }
    return;
  case N_ASSIGN_PAT:
    wvdl_to_pattern(x, n->a, binding);
    return;
  case N_ASSIGN:
    if (!wvdl_op_is(n->op, "=")) {
      break;
    }
    n->type = N_ASSIGN_PAT;
    wvdl_to_pattern(x, n->a, binding);
    return;
  case N_ARRAY:
  case N_ARRAY_PAT:
    n->type = N_ARRAY_PAT;
    for (p = n->list; p != NULL; p = p->next) {
      if (p->type == N_SPREAD) {
        p->type = N_REST;
        wvdl_to_pattern(x, p->a, binding);
      } else if (p->type != N_HOLE) {
        wvdl_to_pattern(x, p, binding);
      }
    }
    return;
  case N_OBJECT:
  case N_OBJECT_PAT:
    n->type = N_OBJECT_PAT;
    for (p = n->list; p != NULL; p = p->next) {
      if (p->op == P_SPREAD) {
        wvdl_to_pattern(x, p->b, binding);
      } else if ((p->op == P_INIT) || (p->op == P_SHORT)) {
        wvdl_to_pattern(x, p->b, binding);
      } else {
        wvdl_error(x, p->pos, "invalid destructuring");
      }
    }
    return;
  default:
    break;
  }
  if (n->paren) {
    wvdl_error(x, n->pos, "invalid destructuring target");
  }
  wvdl_error(x, n->pos, "invalid assignment target");
}

static wvdl_node *wvdl_assign(struct wvdl *x, int no_in) {
  int pos = x->tok.start;
  wvdl_node *left;
  if (wvdl_is_name(x, "yield") && x->in_generator) {
    wvdl_node *n = wvdl_new(x, N_YIELD, pos);
    wvdl_next(x);
    if (wvdl_eat(x, "*")) {
      n->flags = 1; /* yield* */
      n->a = wvdl_assign(x, no_in);
    } else if (!x->tok.nl && !wvdl_is(x, ")") && !wvdl_is(x, "]") &&
               !wvdl_is(x, "}") && !wvdl_is(x, ",") && !wvdl_is(x, ";") &&
               !wvdl_is(x, ":") && (x->tok.type != WVDL_T_EOF)) {
      n->a = wvdl_assign(x, no_in);
    }
    return n;
  }
  left = wvdl_conditional(x, no_in);
  if (x->tok.type == WVDL_T_PUNCT) {
    static const char *const ops[] = {"=",   "+=",  "-=",  "*=",   "/=",
                                      "%=",  "<<=", ">>=", ">>>=", "&=",
                                      "|=",  "^=",  "**=", "&&=",  "||=",
                                      "?\?=", NULL};
    int i;
    for (i = 0; ops[i] != NULL; i++) {
      if (wvdl_is(x, ops[i])) {
        wvdl_node *n = wvdl_new(x, N_ASSIGN, x->tok.start);
        n->op = wvdl_tok_op(x);
        if (i >= 12) {
          x->modern = 1; /* **= &&= ||= ??= */
        }
        if (i == 0) {
          if ((left->type == N_ARRAY) || (left->type == N_OBJECT)) {
            wvdl_to_pattern(x, left, 0);
          } else if ((left->type != N_ID) && (left->type != N_MEMBER)) {
            wvdl_error(x, left->pos, "invalid assignment target");
          }
        } else if ((left->type != N_ID) && (left->type != N_MEMBER)) {
          wvdl_error(x, left->pos, "invalid assignment target");
        }
        wvdl_next(x);
        n->a = left;
        n->b = wvdl_assign(x, no_in);
        return n;
      }
    }
  }
  return left;
}

static wvdl_node *wvdl_expr(struct wvdl *x, int no_in) {
  wvdl_node *e = wvdl_assign(x, no_in);
  if (wvdl_is(x, ",")) {
    wvdl_node *seq = wvdl_new(x, N_SEQ, e->pos);
    wvdl_node *tail = e;
    seq->list = e;
    while (wvdl_eat(x, ",")) {
      wvdl_node *n = wvdl_assign(x, no_in);
      tail->next = n;
      tail = n;
    }
    return seq;
  }
  return e;
}

/* a binding: name, [pattern], {pattern} */
static wvdl_node *wvdl_binding_target(struct wvdl *x) {
  wvdl_node *n;
  if (wvdl_is(x, "[")) {
    n = wvdl_array_literal(x);
    wvdl_to_pattern(x, n, 1);
    return n;
  }
  if (wvdl_is(x, "{")) {
    n = wvdl_object_literal(x);
    wvdl_to_pattern(x, n, 1);
    return n;
  }
  if (wvdl_tok_reserved(x)) {
    wvdl_error(x, x->tok.start, "unexpected keyword");
  }
  return wvdl_ident(x);
}

/* ------------------------------------------------------------- classes */

static wvdl_node *wvdl_class(struct wvdl *x, int is_decl) {
  wvdl_node *c = wvdl_new(x, N_CLASS, x->tok.start);
  wvdl_node *head = NULL, *tail = NULL;
  int saved = x->in_class;
  wvdl_next(x); /* class */
  if ((x->tok.type == WVDL_T_NAME) && !wvdl_is_name(x, "extends")) {
    c->a = wvdl_ident(x);
  } else if (is_decl) {
    wvdl_error(x, x->tok.start, "expected the class name");
  }
  if (wvdl_is_name(x, "extends")) {
    wvdl_next(x);
    c->b = wvdl_lhs(x, 0);
  }
  c->flags = is_decl ? WVDL_F_DECL : 0;
  x->in_class = 1;
  wvdl_expect(x, "{");
  while (!wvdl_is(x, "}")) {
    wvdl_node *m;
    int computed = 0, flags = 0;
    if (wvdl_eat(x, ";")) {
      continue;
    }
    m = wvdl_new(x, N_MEMBER_DEF, x->tok.start);
    m->op = P_METHOD;
    if (wvdl_is_name(x, "static")) {
      struct wvdl_mark mk;
      wvdl_save(x, &mk);
      wvdl_next(x);
      if (wvdl_is(x, "{")) {
        m->op = P_STATIC_BLOCK;
        m->is_static = 1;
        {
          int sf = x->in_function;
          x->in_function = 1;
          m->b = wvdl_block_body(x);
          x->in_function = sf;
        }
        wvdl_append(&head, &tail, m);
        continue;
      }
      if (!wvdl_is(x, "(") && !wvdl_is(x, "=") && !wvdl_is(x, ";") &&
          !wvdl_is(x, "}")) {
        m->is_static = 1;
      } else {
        wvdl_restore(x, &mk);
      }
    }
    if (wvdl_is_name(x, "async") || wvdl_is_name(x, "get") ||
        wvdl_is_name(x, "set")) {
      struct wvdl_mark mk;
      int word = wvdl_is_name(x, "async") ? 1 : wvdl_is_name(x, "get") ? 2 : 3;
      wvdl_save(x, &mk);
      wvdl_next(x);
      if (!wvdl_is(x, "(") && !wvdl_is(x, "=") && !wvdl_is(x, ";") &&
          !wvdl_is(x, "}") && !x->tok.nl) {
        if (word == 1) {
          flags |= WVDL_F_ASYNC;
        } else {
          m->op = (word == 2) ? P_GET : P_SET;
        }
      } else {
        wvdl_restore(x, &mk);
      }
    }
    if (wvdl_eat(x, "*")) {
      flags |= WVDL_F_GEN;
    }
    m->a = wvdl_prop_key(x, &computed);
    m->flags = computed ? WVDL_F_COMPUTED : 0;
    if ((m->op == P_GET) || (m->op == P_SET)) {
      wvdl_note_accessor(m->a, computed);
    }
    if (wvdl_is(x, "(")) {
      if (!computed && !m->is_static && (m->a->type == N_ID) &&
          (m->a->len == 11) && (memcmp(m->a->str, "constructor", 11) == 0) &&
          (m->op == P_METHOD)) {
        m->op = P_CTOR;
      }
      m->b = wvdl_method(x, flags, m->pos);
    } else {
      /* a field: name = value; */
      m->op = P_FIELD;
      if (wvdl_eat(x, "=")) {
        int sf = x->in_function;
        x->in_function = 1;
        m->b = wvdl_assign(x, 0);
        x->in_function = sf;
      }
      wvdl_semicolon(x);
    }
    wvdl_append(&head, &tail, m);
  }
  wvdl_next(x);
  x->in_class = saved;
  c->list = head;
  return c;
}

/* ---------------------------------------------------------- statements */

static wvdl_node *wvdl_var(struct wvdl *x, int kind, int no_in) {
  wvdl_node *v = wvdl_new(x, N_VAR, x->tok.start);
  wvdl_node *head = NULL, *tail = NULL;
  v->op = kind;
  if (kind != K_VAR) {
    x->modern = 1;
  }
  wvdl_next(x); /* var / let / const */
  do {
    wvdl_node *d = wvdl_new(x, N_DECL, x->tok.start);
    d->a = wvdl_binding_target(x);
    if (wvdl_eat(x, "=")) {
      d->b = wvdl_assign(x, no_in);
    }
    wvdl_append(&head, &tail, d);
  } while (wvdl_eat(x, ","));
  v->list = head;
  return v;
}

/* `let` starts a declaration here (and is not a name being used)? */
static int wvdl_let_declaration(struct wvdl *x) {
  struct wvdl_mark m;
  int yes;
  if (!wvdl_is_name(x, "let")) {
    return 0;
  }
  wvdl_save(x, &m);
  wvdl_next(x);
  yes = ((x->tok.type == WVDL_T_NAME) && !wvdl_is_name(x, "in") &&
         !wvdl_is_name(x, "instanceof")) ||
        wvdl_is(x, "[") || wvdl_is(x, "{");
  wvdl_restore(x, &m);
  return yes;
}

static wvdl_node *wvdl_for(struct wvdl *x) {
  int pos = x->tok.start, is_await = 0;
  wvdl_node *init = NULL, *n;
  wvdl_next(x); /* for */
  if (wvdl_is_name(x, "await") && x->in_async) {
    wvdl_next(x);
    is_await = 1;
  }
  wvdl_expect(x, "(");
  if (wvdl_is(x, ";")) {
    init = NULL;
  } else if (wvdl_is_name(x, "var")) {
    init = wvdl_var(x, K_VAR, 1);
  } else if (wvdl_let_declaration(x)) {
    init = wvdl_var(x, K_LET, 1);
  } else if (wvdl_is_name(x, "const")) {
    init = wvdl_var(x, K_CONST, 1);
  } else {
    init = wvdl_expr(x, 1);
  }
  if ((init != NULL) && (wvdl_is_name(x, "of") || wvdl_is_name(x, "in"))) {
    int of = wvdl_is_name(x, "of");
    if (is_await && !of) {
      wvdl_error(x, x->tok.start, "expected 'of' after for await");
    }
    n = wvdl_new(x, of ? N_FOR_OF : N_FOR_IN, pos);
    n->flags = is_await; /* for await (... of ...) */
    if ((init->type != N_VAR) &&
        ((init->type == N_ARRAY) || (init->type == N_OBJECT))) {
      wvdl_to_pattern(x, init, 0);
    }
    wvdl_next(x);
    n->a = init;
    n->b = of ? wvdl_assign(x, 0) : wvdl_expr(x, 0);
    wvdl_expect(x, ")");
    n->d = wvdl_statement(x);
    return n;
  }
  if (is_await) {
    wvdl_error(x, pos, "expected 'of' after for await");
  }
  n = wvdl_new(x, N_FOR, pos);
  n->a = init;
  wvdl_expect(x, ";");
  if (!wvdl_is(x, ";")) {
    n->b = wvdl_expr(x, 0);
  }
  wvdl_expect(x, ";");
  if (!wvdl_is(x, ")")) {
    n->c = wvdl_expr(x, 0);
  }
  wvdl_expect(x, ")");
  n->d = wvdl_statement(x);
  return n;
}

static wvdl_node *wvdl_statement(struct wvdl *x) {
  int pos = x->tok.start;
  wvdl_node *n;
  if (wvdl_is(x, "{")) {
    return wvdl_block_body(x);
  }
  if (wvdl_eat(x, ";")) {
    return wvdl_new(x, N_EMPTY, pos);
  }
  if (x->tok.type == WVDL_T_NAME) {
    if (wvdl_is_name(x, "var")) {
      n = wvdl_var(x, K_VAR, 0);
      wvdl_semicolon(x);
      return n;
    }
    if (wvdl_let_declaration(x)) {
      n = wvdl_var(x, K_LET, 0);
      wvdl_semicolon(x);
      return n;
    }
    if (wvdl_is_name(x, "const")) {
      n = wvdl_var(x, K_CONST, 0);
      wvdl_semicolon(x);
      return n;
    }
    if (wvdl_is_name(x, "function")) {
      int flags = WVDL_F_DECL;
      wvdl_node *name, *f;
      wvdl_next(x);
      if (wvdl_eat(x, "*")) {
        flags |= WVDL_F_GEN;
      }
      name = wvdl_ident(x);
      f = wvdl_function(x, flags, pos);
      f->a = name;
      return f;
    }
    if (wvdl_is_name(x, "async")) {
      struct wvdl_mark m;
      wvdl_save(x, &m);
      wvdl_next(x);
      if (wvdl_is_name(x, "function") && !x->tok.nl) {
        int flags = WVDL_F_DECL | WVDL_F_ASYNC;
        wvdl_node *name, *f;
        wvdl_next(x);
        if (wvdl_eat(x, "*")) {
          flags |= WVDL_F_GEN;
        }
        name = wvdl_ident(x);
        f = wvdl_function(x, flags, pos);
        f->a = name;
        return f;
      }
      wvdl_restore(x, &m);
    }
    if (wvdl_is_name(x, "class")) {
      return wvdl_class(x, 1);
    }
    if (wvdl_is_name(x, "if")) {
      n = wvdl_new(x, N_IF, pos);
      wvdl_next(x);
      wvdl_expect(x, "(");
      n->a = wvdl_expr(x, 0);
      wvdl_expect(x, ")");
      n->b = wvdl_statement(x);
      if (wvdl_is_name(x, "else")) {
        wvdl_next(x);
        n->c = wvdl_statement(x);
      }
      return n;
    }
    if (wvdl_is_name(x, "for")) {
      return wvdl_for(x);
    }
    if (wvdl_is_name(x, "while")) {
      n = wvdl_new(x, N_WHILE, pos);
      wvdl_next(x);
      wvdl_expect(x, "(");
      n->a = wvdl_expr(x, 0);
      wvdl_expect(x, ")");
      n->d = wvdl_statement(x);
      return n;
    }
    if (wvdl_is_name(x, "do")) {
      n = wvdl_new(x, N_DO, pos);
      wvdl_next(x);
      n->d = wvdl_statement(x);
      if (!wvdl_is_name(x, "while")) {
        wvdl_error(x, x->tok.start, "expected 'while'");
      }
      wvdl_next(x);
      wvdl_expect(x, "(");
      n->a = wvdl_expr(x, 0);
      wvdl_expect(x, ")");
      wvdl_eat(x, ";"); /* optional even on one line, after do-while */
      return n;
    }
    if (wvdl_is_name(x, "return")) {
      n = wvdl_new(x, N_RETURN, pos);
      wvdl_next(x);
      if (!wvdl_is(x, ";") && !wvdl_is(x, "}") && !x->tok.nl &&
          (x->tok.type != WVDL_T_EOF)) {
        n->a = wvdl_expr(x, 0);
      }
      wvdl_semicolon(x);
      return n;
    }
    if (wvdl_is_name(x, "break") || wvdl_is_name(x, "continue")) {
      n = wvdl_new(x, wvdl_is_name(x, "break") ? N_BREAK : N_CONTINUE, pos);
      wvdl_next(x);
      if ((x->tok.type == WVDL_T_NAME) && !x->tok.nl && !wvdl_tok_reserved(x)) {
        n->str = x->src + x->tok.start;
        n->len = x->tok.end - x->tok.start;
        wvdl_next(x);
      }
      wvdl_semicolon(x);
      return n;
    }
    if (wvdl_is_name(x, "throw")) {
      n = wvdl_new(x, N_THROW, pos);
      wvdl_next(x);
      n->a = wvdl_expr(x, 0);
      wvdl_semicolon(x);
      return n;
    }
    if (wvdl_is_name(x, "try")) {
      n = wvdl_new(x, N_TRY, pos);
      wvdl_next(x);
      n->a = wvdl_block_body(x);
      if (wvdl_is_name(x, "catch")) {
        wvdl_next(x);
        if (wvdl_eat(x, "(")) {
          n->b = wvdl_binding_target(x);
          wvdl_expect(x, ")");
        } else {
          /* catch { } without a binding: give it one */
          x->modern = 1;
          n->b = wvdl_new(x, N_ID, x->tok.start);
          n->b->str = "$wv_e";
          n->b->len = 5;
        }
        n->c = wvdl_block_body(x);
      }
      if (wvdl_is_name(x, "finally")) {
        wvdl_next(x);
        n->d = wvdl_block_body(x);
      }
      if ((n->c == NULL) && (n->d == NULL)) {
        wvdl_error(x, pos, "try without catch or finally");
      }
      return n;
    }
    if (wvdl_is_name(x, "switch")) {
      wvdl_node *head = NULL, *tail = NULL;
      n = wvdl_new(x, N_SWITCH, pos);
      wvdl_next(x);
      wvdl_expect(x, "(");
      n->a = wvdl_expr(x, 0);
      wvdl_expect(x, ")");
      wvdl_expect(x, "{");
      while (!wvdl_is(x, "}")) {
        wvdl_node *c = wvdl_new(x, N_CASE, x->tok.start);
        wvdl_node *sh = NULL, *st = NULL;
        if (wvdl_is_name(x, "case")) {
          wvdl_next(x);
          c->a = wvdl_expr(x, 0);
        } else if (wvdl_is_name(x, "default")) {
          wvdl_next(x);
        } else {
          wvdl_error(x, x->tok.start, "expected 'case'");
        }
        wvdl_expect(x, ":");
        while (!wvdl_is(x, "}") && !wvdl_is_name(x, "case") &&
               !wvdl_is_name(x, "default")) {
          wvdl_append(&sh, &st, wvdl_statement(x));
        }
        c->list = sh;
        wvdl_append(&head, &tail, c);
      }
      wvdl_next(x);
      n->list = head;
      return n;
    }
    if (wvdl_is_name(x, "debugger")) {
      wvdl_next(x);
      wvdl_semicolon(x);
      return wvdl_new(x, N_DEBUGGER, pos);
    }
    if (wvdl_is_name(x, "import") || wvdl_is_name(x, "export")) {
      struct wvdl_mark m;
      wvdl_save(x, &m);
      wvdl_next(x);
      if (!wvdl_is(x, "(") && !wvdl_is(x, ".")) {
        wvdl_error(x, pos, "modules (import / export) cannot run on this "
                           "engine; bundle the script first");
      }
      wvdl_restore(x, &m);
    }
    if (wvdl_is_name(x, "with")) {
      n = wvdl_new(x, N_LABEL, pos); /* with: kept, printed as is */
      n->op = 1;
      wvdl_next(x);
      wvdl_expect(x, "(");
      n->a = wvdl_expr(x, 0);
      wvdl_expect(x, ")");
      n->b = wvdl_statement(x);
      return n;
    }
    /* label: */
    {
      struct wvdl_mark m;
      wvdl_save(x, &m);
      if (!wvdl_tok_reserved(x)) {
        int s = x->tok.start, e = x->tok.end;
        wvdl_next(x);
        if (wvdl_is(x, ":")) {
          wvdl_next(x);
          n = wvdl_new(x, N_LABEL, pos);
          n->str = x->src + s;
          n->len = e - s;
          n->b = wvdl_statement(x);
          return n;
        }
      }
      wvdl_restore(x, &m);
    }
  }
  n = wvdl_new(x, N_EXPR_STMT, pos);
  n->a = wvdl_expr(x, 0);
  wvdl_semicolon(x);
  return n;
}

static wvdl_node *wvdl_program(struct wvdl *x) {
  wvdl_node *p = wvdl_new(x, N_PROGRAM, 0);
  wvdl_node *head = NULL, *tail = NULL;
  if ((x->len >= 2) && (x->src[0] == '#') && (x->src[1] == '!')) {
    while ((x->pos < x->len) && !wvdl_newline_at(x, x->pos)) {
      x->pos++; /* #!/usr/bin/env node */
    }
    x->modern = 1;
  }
  wvdl_next(x);
  while (x->tok.type != WVDL_T_EOF) {
    wvdl_append(&head, &tail, wvdl_statement(x));
  }
  p->list = head;
  return p;
}

/* ================================================================ scopes */

/* What each name refers to, worked out before printing:
   - let / const / class become var, so a block binding is renamed when
     the function already has that name (or reads an outer one by it);
   - a block binding used by a function made inside a loop needs a fresh
     copy per turn: that loop's body becomes a function called each turn;
   - the head binding of such a loop, assigned in the body, is copied back. */

enum { B_VAR, B_LET, B_CONST, B_CLASS, B_FUNC, B_PARAM, B_CATCH,
       B_BLOCK_FUNC, B_SELF };

struct wvdl_binding {
  const char *name;
  int len;
  int kind;
  struct wvdl_scope *scope;
  const char *out; /* the name printed */
  int renamed;
  int captured;         /* used from a function inside its scope */
  int assigned_in_body; /* a for (let ...) binding assigned in the body */
  struct wvdl_binding *next;     /* in its scope */
  struct wvdl_binding *next_all; /* in x->blocked */
  struct wvdl_binding *next_any; /* in x->bindings */
};

struct wvdl_scope {
  struct wvdl_scope *up;
  struct wvdl_scope *fn; /* the function scope it belongs to */
  wvdl_node *loop;       /* innermost loop around it, same function */
  int is_fn;
  int is_head; /* the scope of a for (let ...) head */
  int in_body; /* the resolver is inside that loop's body */
  struct wvdl_binding *first, *last;
};

enum { NM_BIND, NM_DECL, NM_FREE };

struct wvdl_name {
  const void *owner; /* a scope */
  const char *s;
  int n;
  int tag;
  int count;
  void *ptr;
};

#define WVDL_X_IIFE 1 /* loop: the body runs as a function per turn */
#define WVDL_X_DONE 2 /* already printed (paired accessor, hoisted function) */
#define WVDL_X_YIELD 4 /* holds a yield / await of the function printed */

static unsigned wvdl_name_hash(const void *owner, const char *s, int n,
                               int tag) {
  unsigned h = 2166136261u;
  size_t p = (size_t)owner;
  int i;
  for (i = 0; i < n; i++) {
    h = (h ^ (unsigned char)s[i]) * 16777619u;
  }
  h ^= (unsigned)tag * 0x9E3779B9u;
  h ^= (unsigned)(p >> 3) * 2654435761u;
  h ^= h >> 15;
  return h;
}

/* entries move when the table grows: never keep one across an add */
static struct wvdl_name *wvdl_name_find(struct wvdl *x, const void *owner,
                                        const char *s, int n, int tag,
                                        int add) {
  struct wvdl_name *e;
  unsigned i, mask;
  if (add && ((x->names_used + 1) * 2 > x->names_cap)) {
    unsigned cap = x->names_cap ? x->names_cap * 2 : 4096, j;
    struct wvdl_name *t = (struct wvdl_name *)wvdl_alloc(
        &x->arena, cap * sizeof(struct wvdl_name));
    if (t == NULL) {
      wvdl_error(x, 0, "out of memory");
    }
    for (j = 0; j < x->names_cap; j++) {
      if (x->names[j].s != NULL) {
        struct wvdl_name *o = &x->names[j];
        unsigned k = wvdl_name_hash(o->owner, o->s, o->n, o->tag) & (cap - 1);
        while (t[k].s != NULL) {
          k = (k + 1) & (cap - 1);
        }
        t[k] = *o;
      }
    }
    x->names = t;
    x->names_cap = cap;
  }
  if (x->names_cap == 0) {
    return NULL;
  }
  mask = x->names_cap - 1;
  i = wvdl_name_hash(owner, s, n, tag) & mask;
  while ((e = &x->names[i])->s != NULL) {
    if ((e->owner == owner) && (e->tag == tag) && (e->n == n) &&
        (memcmp(e->s, s, (size_t)n) == 0)) {
      return e;
    }
    i = (i + 1) & mask;
  }
  if (!add) {
    return NULL;
  }
  e->owner = owner;
  e->s = s;
  e->n = n;
  e->tag = tag;
  x->names_used++;
  return e;
}

static struct wvdl_scope *wvdl_scope_open(struct wvdl *x, struct wvdl_scope *up,
                                          int is_fn, wvdl_node *loop) {
  struct wvdl_scope *s =
      (struct wvdl_scope *)wvdl_alloc(&x->arena, sizeof(struct wvdl_scope));
  if (s == NULL) {
    wvdl_error(x, 0, "out of memory");
  }
  s->up = up;
  s->is_fn = is_fn;
  s->fn = is_fn ? s : up->fn;
  s->loop = is_fn ? NULL : loop;
  return s;
}

static struct wvdl_binding *wvdl_declare(struct wvdl *x, struct wvdl_scope *s,
                                         const char *name, int len, int kind) {
  struct wvdl_name *e = wvdl_name_find(x, s, name, len, NM_BIND, 1);
  struct wvdl_binding *b = (struct wvdl_binding *)e->ptr;
  if (b != NULL) { /* var x; var x; - one variable */
    if (b->kind == B_SELF) {
      b->kind = kind;
    }
    return b;
  }
  b = (struct wvdl_binding *)wvdl_alloc(&x->arena, sizeof(struct wvdl_binding));
  if (b == NULL) {
    wvdl_error(x, 0, "out of memory");
  }
  e->ptr = b;
  b->name = name;
  b->len = len;
  b->kind = kind;
  b->scope = s;
  b->out = wvdl_strdup(x, name, len);
  if (s->last != NULL) {
    s->last->next = b;
  } else {
    s->first = b;
  }
  s->last = b;
  /* a class's own name lives inside its function in the output */
  if ((kind != B_SELF) || s->is_fn) {
    wvdl_name_find(x, s->fn, name, len, NM_DECL, 1)->count++;
  }
  if (!s->is_fn && (kind != B_SELF)) {
    b->next_all = x->blocked;
    x->blocked = b;
  }
  b->next_any = x->bindings;
  x->bindings = b;
  return b;
}

/* ---------------------------------------------- pass 1: declare names */

static void wvdl_w1(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s,
                    wvdl_node *loop);

static void wvdl_w1_list(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s,
                         wvdl_node *loop) {
  for (; n != NULL; n = n->next) {
    wvdl_w1(x, n, s, loop);
  }
}

/* names of a pattern go into `d` (none when d is NULL: an assignment);
   default values are walked in `s` */
static void wvdl_w1_pattern(struct wvdl *x, wvdl_node *p, struct wvdl_scope *d,
                            int kind, struct wvdl_scope *s, wvdl_node *loop) {
  wvdl_node *q;
  if (p == NULL) {
    return;
  }
  switch (p->type) {
  case N_ID:
    if (d != NULL) {
      p->binding = wvdl_declare(x, d, p->str, p->len, kind);
    }
    return;
  case N_ASSIGN_PAT:
    wvdl_w1_pattern(x, p->a, d, kind, s, loop);
    wvdl_w1(x, p->b, s, loop);
    return;
  case N_REST:
    wvdl_w1_pattern(x, p->a, d, kind, s, loop);
    return;
  case N_ARRAY_PAT:
    for (q = p->list; q != NULL; q = q->next) {
      if (q->type != N_HOLE) {
        wvdl_w1_pattern(x, q, d, kind, s, loop);
      }
    }
    return;
  case N_OBJECT_PAT:
    for (q = p->list; q != NULL; q = q->next) {
      if (q->flags & WVDL_F_COMPUTED) {
        wvdl_w1(x, q->a, s, loop);
      }
      wvdl_w1_pattern(x, q->b, d, kind, s, loop);
    }
    return;
  default:
    wvdl_w1(x, p, s, loop); /* obj.x in [obj.x] = ... */
    return;
  }
}

static void wvdl_w1_function(struct wvdl *x, wvdl_node *f,
                             struct wvdl_scope *s) {
  struct wvdl_scope *fs = wvdl_scope_open(x, s, 1, NULL);
  wvdl_node *p;
  f->scope = fs;
  if ((f->a != NULL) && !(f->flags & WVDL_F_DECL)) {
    f->a->binding = wvdl_declare(x, fs, f->a->str, f->a->len, B_SELF);
  }
  for (p = f->list; p != NULL; p = p->next) {
    wvdl_w1_pattern(x, p, fs, B_PARAM, fs, NULL);
  }
  if (f->flags & WVDL_F_EXPR_BODY) {
    wvdl_w1(x, f->b, fs, NULL);
  } else {
    wvdl_w1_list(x, f->b->list, fs, NULL);
  }
}

static void wvdl_w1_class(struct wvdl *x, wvdl_node *c, struct wvdl_scope *s,
                          wvdl_node *loop) {
  struct wvdl_scope *cs;
  wvdl_node *m;
  if ((c->flags & WVDL_F_DECL) && (c->a != NULL)) {
    c->a->binding = wvdl_declare(x, s, c->a->str, c->a->len, B_CLASS);
  }
  wvdl_w1(x, c->b, s, loop);
  cs = wvdl_scope_open(x, s, 0, loop);
  c->scope = cs;
  if (c->a != NULL) {
    struct wvdl_binding *self =
        wvdl_declare(x, cs, c->a->str, c->a->len, B_SELF);
    if (!(c->flags & WVDL_F_DECL)) {
      c->a->binding = self;
    }
  }
  for (m = c->list; m != NULL; m = m->next) {
    if (m->flags & WVDL_F_COMPUTED) {
      wvdl_w1(x, m->a, cs, loop);
    }
    if ((m->op == P_FIELD) || (m->op == P_STATIC_BLOCK)) {
      /* field values and static blocks run in a function of their own */
      m->scope = wvdl_scope_open(x, cs, 1, NULL);
      if (m->op == P_FIELD) {
        wvdl_w1(x, m->b, m->scope, NULL);
      } else {
        wvdl_w1_list(x, m->b->list, m->scope, NULL);
      }
    } else {
      wvdl_w1(x, m->b, cs, loop);
    }
  }
}

static void wvdl_w1(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s,
                    wvdl_node *loop) {
  wvdl_node *q;
  if (n == NULL) {
    return;
  }
  switch (n->type) {
  case N_FUNC:
    if (n->flags & WVDL_F_DECL) {
      n->a->binding = wvdl_declare(x, s, n->a->str, n->a->len,
                                   s->is_fn ? B_FUNC : B_BLOCK_FUNC);
    }
    wvdl_w1_function(x, n, s);
    return;
  case N_CLASS:
    wvdl_w1_class(x, n, s, loop);
    return;
  case N_BLOCK:
    n->scope = wvdl_scope_open(x, s, 0, loop);
    wvdl_w1_list(x, n->list, n->scope, loop);
    return;
  case N_VAR:
    for (q = n->list; q != NULL; q = q->next) {
      wvdl_w1_pattern(x, q->a, (n->op == K_VAR) ? s->fn : s,
                      (n->op == K_VAR)   ? B_VAR
                      : (n->op == K_LET) ? B_LET
                                         : B_CONST,
                      s, loop);
      wvdl_w1(x, q->b, s, loop);
    }
    return;
  case N_FOR:
  case N_FOR_IN:
  case N_FOR_OF: {
    struct wvdl_scope *hs = s;
    if ((n->a != NULL) && (n->a->type == N_VAR) && (n->a->op != K_VAR)) {
      hs = wvdl_scope_open(x, s, 0, n);
      hs->is_head = 1;
      n->scope = hs;
    }
    if ((n->type != N_FOR) && (n->a->type != N_VAR)) {
      wvdl_w1_pattern(x, n->a, NULL, 0, hs, n);
    } else {
      wvdl_w1(x, n->a, hs, n);
    }
    wvdl_w1(x, n->b, hs, n);
    wvdl_w1(x, n->c, hs, n);
    wvdl_w1(x, n->d, hs, n);
    return;
  }
  case N_WHILE:
  case N_DO:
    wvdl_w1(x, n->a, s, loop);
    wvdl_w1(x, n->d, s, n);
    return;
  case N_SWITCH:
    wvdl_w1(x, n->a, s, loop);
    n->scope = wvdl_scope_open(x, s, 0, loop);
    for (q = n->list; q != NULL; q = q->next) {
      wvdl_w1(x, q->a, n->scope, loop);
      wvdl_w1_list(x, q->list, n->scope, loop);
    }
    return;
  case N_TRY:
    wvdl_w1(x, n->a, s, loop);
    if (n->c != NULL) {
      n->scope = wvdl_scope_open(x, s, 0, loop);
      wvdl_w1_pattern(x, n->b, n->scope, B_CATCH, n->scope, loop);
      wvdl_w1(x, n->c, n->scope, loop);
    }
    wvdl_w1(x, n->d, s, loop);
    return;
  case N_PROP:
    if (n->flags & WVDL_F_COMPUTED) {
      wvdl_w1(x, n->a, s, loop);
    }
    wvdl_w1(x, n->b, s, loop);
    return;
  case N_MEMBER:
    wvdl_w1(x, n->a, s, loop);
    if (n->flags & WVDL_F_COMPUTED) {
      wvdl_w1(x, n->b, s, loop);
    }
    return;
  case N_ID: case N_NUM: case N_STR: case N_REGEX: case N_THIS: case N_SUPER:
  case N_LIT: case N_HOLE: case N_PRIVATE_NAME: case N_NEW_TARGET: case N_RAW:
  case N_EMPTY: case N_DEBUGGER: case N_BREAK: case N_CONTINUE:
    return;
  default:
    wvdl_w1(x, n->a, s, loop);
    wvdl_w1(x, n->b, s, loop);
    wvdl_w1(x, n->c, s, loop);
    wvdl_w1(x, n->d, s, loop);
    wvdl_w1_list(x, n->list, s, loop);
    return;
  }
}

/* ------------------------------------------- pass 2: resolve the uses */

static void wvdl_resolve(struct wvdl *x, wvdl_node *id, struct wvdl_scope *s,
                         int assigning) {
  struct wvdl_scope *t;
  struct wvdl_binding *b = NULL;
  int crossed = 0;
  for (t = s; t != NULL; t = t->up) {
    struct wvdl_name *e = wvdl_name_find(x, t, id->str, id->len, NM_BIND, 0);
    if ((e != NULL) && (e->ptr != NULL)) {
      b = (struct wvdl_binding *)e->ptr;
      break;
    }
  }
  /* every function between the use and the binding reads it from outside */
  for (t = s; (t != NULL) && ((b == NULL) || (t != b->scope)); t = t->up) {
    if (t->is_fn) {
      wvdl_name_find(x, t, id->str, id->len, NM_FREE, 1);
      crossed = 1;
    }
  }
  if (b != NULL) {
    id->binding = b;
    if (crossed) {
      b->captured = 1;
    }
    if (assigning && b->scope->is_head && b->scope->in_body) {
      b->assigned_in_body = 1;
    }
  }
}

static void wvdl_w2(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s);

static void wvdl_w2_list(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s) {
  for (; n != NULL; n = n->next) {
    wvdl_w2(x, n, s);
  }
}

static void wvdl_w2_target(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s) {
  wvdl_node *q;
  if (n == NULL) {
    return;
  }
  switch (n->type) {
  case N_ID:
    wvdl_resolve(x, n, s, 1);
    return;
  case N_ASSIGN_PAT:
    wvdl_w2_target(x, n->a, s);
    wvdl_w2(x, n->b, s);
    return;
  case N_REST:
    wvdl_w2_target(x, n->a, s);
    return;
  case N_ARRAY_PAT:
    for (q = n->list; q != NULL; q = q->next) {
      if (q->type != N_HOLE) {
        wvdl_w2_target(x, q, s);
      }
    }
    return;
  case N_OBJECT_PAT:
    for (q = n->list; q != NULL; q = q->next) {
      if (q->flags & WVDL_F_COMPUTED) {
        wvdl_w2(x, q->a, s);
      }
      wvdl_w2_target(x, q->b, s);
    }
    return;
  default:
    wvdl_w2(x, n, s);
    return;
  }
}

static void wvdl_w2(struct wvdl *x, wvdl_node *n, struct wvdl_scope *s) {
  wvdl_node *q;
  if (n == NULL) {
    return;
  }
  switch (n->type) {
  case N_ID:
    wvdl_resolve(x, n, s, 0);
    return;
  case N_FUNC:
    for (q = n->list; q != NULL; q = q->next) {
      wvdl_w2_target(x, q, n->scope);
    }
    if (n->flags & WVDL_F_EXPR_BODY) {
      wvdl_w2(x, n->b, n->scope);
    } else {
      wvdl_w2_list(x, n->b->list, n->scope);
    }
    return;
  case N_CLASS:
    wvdl_w2(x, n->b, s);
    for (q = n->list; q != NULL; q = q->next) {
      if (q->flags & WVDL_F_COMPUTED) {
        wvdl_w2(x, q->a, n->scope);
      }
      if (q->op == P_FIELD) {
        wvdl_w2(x, q->b, q->scope);
      } else if (q->op == P_STATIC_BLOCK) {
        wvdl_w2_list(x, q->b->list, q->scope);
      } else {
        wvdl_w2(x, q->b, n->scope);
      }
    }
    return;
  case N_BLOCK:
    wvdl_w2_list(x, n->list, n->scope);
    return;
  case N_VAR:
    for (q = n->list; q != NULL; q = q->next) {
      wvdl_w2_target(x, q->a, s);
      wvdl_w2(x, q->b, s);
    }
    return;
  case N_FOR:
  case N_FOR_IN:
  case N_FOR_OF: {
    struct wvdl_scope *hs = (n->scope != NULL) ? n->scope : s;
    if ((n->type != N_FOR) && (n->a->type != N_VAR)) {
      wvdl_w2_target(x, n->a, hs);
    } else {
      wvdl_w2(x, n->a, hs);
    }
    wvdl_w2(x, n->b, hs);
    wvdl_w2(x, n->c, hs);
    if (n->scope != NULL) {
      n->scope->in_body = 1;
    }
    wvdl_w2(x, n->d, hs);
    if (n->scope != NULL) {
      n->scope->in_body = 0;
    }
    return;
  }
  case N_SWITCH:
    wvdl_w2(x, n->a, s);
    for (q = n->list; q != NULL; q = q->next) {
      wvdl_w2(x, q->a, n->scope);
      wvdl_w2_list(x, q->list, n->scope);
    }
    return;
  case N_TRY:
    wvdl_w2(x, n->a, s);
    if (n->c != NULL) {
      wvdl_w2_target(x, n->b, n->scope);
      wvdl_w2(x, n->c, n->scope);
    }
    wvdl_w2(x, n->d, s);
    return;
  case N_ASSIGN:
    wvdl_w2_target(x, n->a, s);
    wvdl_w2(x, n->b, s);
    return;
  case N_UPDATE:
    wvdl_w2_target(x, n->a, s);
    return;
  case N_PROP:
    if (n->flags & WVDL_F_COMPUTED) {
      wvdl_w2(x, n->a, s);
    }
    wvdl_w2(x, n->b, s);
    return;
  case N_MEMBER:
    wvdl_w2(x, n->a, s);
    if (n->flags & WVDL_F_COMPUTED) {
      wvdl_w2(x, n->b, s);
    }
    return;
  case N_NUM: case N_STR: case N_REGEX: case N_THIS: case N_SUPER: case N_LIT:
  case N_HOLE: case N_PRIVATE_NAME: case N_NEW_TARGET: case N_RAW:
  case N_EMPTY: case N_DEBUGGER: case N_BREAK: case N_CONTINUE:
    return;
  default:
    wvdl_w2(x, n->a, s);
    wvdl_w2(x, n->b, s);
    wvdl_w2(x, n->c, s);
    wvdl_w2(x, n->d, s);
    wvdl_w2_list(x, n->list, s);
    return;
  }
}

static const char *wvdl_fmt(struct wvdl *x, const char *fmt, ...) {
  va_list ap;
  int n;
  char *p;
  va_start(ap, fmt);
  n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (n < 0) {
    wvdl_error(x, 0, "format failed");
  }
  p = (char *)wvdl_alloc(&x->arena, (size_t)n + 1);
  if (p == NULL) {
    wvdl_error(x, 0, "out of memory");
  }
  va_start(ap, fmt);
  vsnprintf(p, (size_t)n + 1, fmt, ap);
  va_end(ap);
  return p;
}

/* renames, and the loops whose body must run as a function per turn */
static void wvdl_plan(struct wvdl *x) {
  struct wvdl_binding *b;
  for (b = x->blocked; b != NULL; b = b->next_all) {
    struct wvdl_scope *f = b->scope->fn;
    struct wvdl_name *d = wvdl_name_find(x, f, b->name, b->len, NM_DECL, 0);
    struct wvdl_name *u = wvdl_name_find(x, f, b->name, b->len, NM_FREE, 0);
    if (b->kind == B_BLOCK_FUNC) {
      u = NULL; /* old scripts call them from outside the block (Annex B) */
    }
    if (((d != NULL) && (d->count > 1)) || (u != NULL)) {
      b->out = wvdl_fmt(x, "%.*s%s%d", b->len, b->name, x->prefix,
                        ++x->counter);
      b->renamed = 1;
    }
    if (b->captured && (b->scope->loop != NULL)) {
      b->scope->loop->extra |= WVDL_X_IIFE;
    }
  }
  /* var char = ...: ES3 kept such words for the future; stay clear of them.
     Globals keep their names, other scripts may use them. */
  for (b = x->bindings; b != NULL; b = b->next_any) {
    if (!b->renamed && (b->scope->fn->up != NULL) && (b->kind != B_SELF) &&
        wvdl_in_list(wvdl_future_words, b->name, b->len)) {
      b->out = wvdl_fmt(x, "%.*s%s%d", b->len, b->name, x->prefix,
                        ++x->counter);
      b->renamed = 1;
    }
  }
}

/* =============================================================== printer */

enum { PR_SEQ = 1, PR_ASSIGN, PR_COND, PR_OR, PR_AND, PR_BOR, PR_BXOR,
       PR_BAND, PR_EQ, PR_REL, PR_SHIFT, PR_ADD, PR_MUL, PR_UNARY,
       PR_POSTFIX, PR_CALL, PR_MEMBER, PR_PRIMARY };

/* the functions being printed:
   FN_TOP   the script
   FN_FUNC  a function: owns this and arguments
   FN_ARROW an arrow (or a class's wrapper): this and arguments from outside
   FN_LOOP  a loop body run as a function: like an arrow, and its var
            declarations belong to the function around it
   FN_THIS  static field values: `this` is the class; declares nothing
   FN_GEN   the state machine of a generator or async function: entered
            again at every resume, so it keeps nothing of its own; its
            variables and temporaries live in the function around it */
enum { FN_TOP, FN_FUNC, FN_ARROW, FN_LOOP, FN_THIS, FN_GEN };

struct wvdl_code {
  struct wvdl_code *next;
  const char *text; /* "b:label" / "c:label" */
  const char *label;
  int len;
  int is_break;
};

struct wvdl_fn {
  struct wvdl_fn *up;
  int kind;
  struct wvdl_buf vars; /* declared at its top: "a, $wv1, ..." */
  const char *this_alias;
  const char *args_alias;
  const char *this_preset;
  int home; /* a class method: 1 instance, 2 static */
  int is_ctor;
  int codes; /* FN_LOOP: 1 break, 2 return */
  struct wvdl_code *labels;
  const char *ctx; /* FN_GEN: the state machine's context */
};

enum { J_LOOP, J_SWITCH, J_LABEL, J_IIFE, J_FUNC };

struct wvdl_jump {
  struct wvdl_jump *up;
  int kind;
  const char *label;
  int len;
};

struct wvdl_cls {
  struct wvdl_cls *up;
  wvdl_node *node;
  const char *name;
  const char *super_name;
  const char *init_name;
  int id;
};

enum { D_DECL, D_ASSIGN, D_HOIST };

static void wvdl_p_expr(struct wvdl *x, wvdl_node *n, int prec);
static void wvdl_p_stmt(struct wvdl *x, wvdl_node *s);
static void wvdl_p_stmts(struct wvdl *x, wvdl_node *list);
static void wvdl_p_class(struct wvdl *x, wvdl_node *c, const char *hint);
static void wvdl_p_destructure(struct wvdl *x, wvdl_node *p, const char *value,
                               int mode, int *first);

/* ---------------------------------------------------------- output */

static void wvdl_o(struct wvdl *x, const char *s) {
  wvdl_put(x->out, s, strlen(s));
}

static void wvdl_on(struct wvdl *x, const char *s, int n) {
  wvdl_put(x->out, s, (size_t)n);
}

struct wvdl_cap {
  struct wvdl_buf buf;
  struct wvdl_buf *saved;
};

static void wvdl_cap_begin(struct wvdl *x, struct wvdl_cap *c) {
  memset(&c->buf, 0, sizeof(c->buf));
  c->buf.arena = &x->arena;
  c->saved = x->out;
  x->out = &c->buf;
}

static const char *wvdl_cap_end(struct wvdl *x, struct wvdl_cap *c) {
  x->out = c->saved;
  if (c->buf.failed) {
    wvdl_error(x, 0, "out of memory");
  }
  return (c->buf.p != NULL) ? c->buf.p : "";
}

static void wvdl_lines_init(struct wvdl *x) {
  int i, n = 1;
  for (i = 0; i < x->len; i++) {
    if (x->src[i] == '\n') {
      n++;
    }
  }
  x->line_starts = (int *)wvdl_alloc(&x->arena, (size_t)n * sizeof(int));
  if (x->line_starts == NULL) {
    wvdl_error(x, 0, "out of memory");
  }
  x->line_count = n;
  n = 0;
  x->line_starts[n++] = 0;
  for (i = 0; i < x->len; i++) {
    if (x->src[i] == '\n') {
      x->line_starts[n++] = i + 1;
    }
  }
  x->line = 1;
}

static int wvdl_line_of(struct wvdl *x, int pos) {
  int lo = 0, hi = x->line_count - 1;
  while (lo < hi) {
    int mid = (lo + hi + 1) / 2;
    if (x->line_starts[mid] <= pos) {
      lo = mid;
    } else {
      hi = mid - 1;
    }
  }
  return lo + 1;
}

/* go down to the line of `pos` in the source, with its indentation */
static void wvdl_line_to(struct wvdl *x, int pos) {
  int line, i;
  if (pos < 0) {
    return;
  }
  line = wvdl_line_of(x, pos);
  if (line <= x->line) {
    return;
  }
  while (x->line < line) {
    wvdl_on(x, "\n", 1);
    x->line++;
  }
  for (i = x->line_starts[line - 1];
       (i < x->len) && ((x->src[i] == ' ') || (x->src[i] == '\t')); i++) {
    wvdl_on(x, x->src + i, 1);
  }
}

static void wvdl_stmt_start(struct wvdl *x, int pos) {
  char c;
  wvdl_line_to(x, pos);
  c = (x->out->len > 0) ? x->out->p[x->out->len - 1] : 0;
  if ((c != '\n') && (c != ' ') && (c != '\t')) {
    wvdl_o(x, " ");
  }
}

/* ------------------------------------------------------------ names */

static const char *wvdl_fresh(struct wvdl *x) {
  return wvdl_fmt(x, "%s%d", x->prefix, ++x->counter);
}

static void wvdl_var_add(struct wvdl_fn *f, const char *name) {
  if (f->vars.len > 0) {
    wvdl_put(&f->vars, ", ", 2);
  }
  wvdl_put(&f->vars, name, strlen(name));
}

/* a temporary, declared at the top of the function being printed */
static const char *wvdl_temp(struct wvdl *x) {
  const char *t = wvdl_fresh(x);
  struct wvdl_fn *f = x->fn;
  while ((f->kind == FN_THIS) || (f->kind == FN_GEN)) {
    f = f->up;
  }
  wvdl_var_add(f, t);
  return t;
}

/* a var inside a loop body run as a function belongs to the function */
static void wvdl_hoist(struct wvdl *x, const char *name) {
  struct wvdl_fn *f = x->fn;
  while ((f->kind == FN_LOOP) || (f->kind == FN_THIS) || (f->kind == FN_GEN)) {
    f = f->up;
  }
  wvdl_var_add(f, name);
}

static int wvdl_is_temp(struct wvdl *x, const char *s) {
  size_t n = strlen(x->prefix);
  if (strncmp(s, x->prefix, n) != 0) {
    return 0;
  }
  for (s += n; *s; s++) {
    if (!isdigit((unsigned char)*s)) {
      return 0;
    }
  }
  return 1;
}

static int wvdl_simple(const char *s) {
  if ((*s == 0) || (*s == '\\') || !wvdl_id_start((unsigned char)*s)) {
    return 0;
  }
  for (s++; *s; s++) {
    if ((*s == '\\') || !wvdl_id_part((unsigned char)*s)) {
      return 0;
    }
  }
  return 1;
}

static int wvdl_quote_name(const char *s, int n) {
  return wvdl_in_list(wvdl_reserved, s, n) ||
         wvdl_in_list(wvdl_es3_reserved, s, n);
}

static const char *wvdl_this_name(struct wvdl *x) {
  struct wvdl_fn *f;
  for (f = x->fn; f != NULL; f = f->up) {
    if (f->this_preset != NULL) {
      return f->this_preset;
    }
    if ((f->kind == FN_FUNC) || (f->kind == FN_TOP)) {
      if (f == x->fn) {
        return "this";
      }
      if (f->this_alias == NULL) {
        f->this_alias = wvdl_fresh(x);
      }
      return f->this_alias;
    }
  }
  return "this";
}

static const char *wvdl_args_name(struct wvdl *x) {
  struct wvdl_fn *f;
  for (f = x->fn; f != NULL; f = f->up) {
    if (f->kind == FN_TOP) {
      return "arguments";
    }
    if (f->kind == FN_FUNC) {
      if (f == x->fn) {
        return "arguments";
      }
      if (f->args_alias == NULL) {
        f->args_alias = wvdl_fresh(x);
      }
      return f->args_alias;
    }
  }
  return "arguments";
}

/* the class method `super` refers to */
static struct wvdl_fn *wvdl_home(struct wvdl *x) {
  struct wvdl_fn *f;
  for (f = x->fn; f != NULL; f = f->up) {
    if (f->home) {
      return f;
    }
    if ((f->kind == FN_FUNC) || (f->kind == FN_TOP)) {
      return NULL;
    }
  }
  return NULL;
}

static int wvdl_cls_declares(wvdl_node *c, wvdl_node *k) {
  wvdl_node *m;
  for (m = c->list; m != NULL; m = m->next) {
    if ((m->a != NULL) && (m->a->type == N_PRIVATE_NAME) &&
        (m->a->len == k->len) &&
        (memcmp(m->a->str, k->str, (size_t)k->len) == 0)) {
      return 1;
    }
  }
  return 0;
}

/* #name -> $wv_<class>_name, a plain property nobody else uses */
static const char *wvdl_private(struct wvdl *x, wvdl_node *k) {
  struct wvdl_cls *c;
  for (c = x->cls; c != NULL; c = c->up) {
    if (wvdl_cls_declares(c->node, k)) {
      return wvdl_fmt(x, "%s_%d_%.*s", x->prefix, c->id, k->len, k->str);
    }
  }
  wvdl_error(x, k->pos, "unknown private name #%.*s", k->len, k->str);
  return NULL;
}

static const char *wvdl_expr_str(struct wvdl *x, wvdl_node *n, int prec) {
  struct wvdl_cap c;
  wvdl_cap_begin(x, &c);
  wvdl_p_expr(x, n, prec);
  return wvdl_cap_end(x, &c);
}

static int wvdl_open(struct wvdl *x, int yes) {
  if (yes) {
    wvdl_o(x, "(");
  }
  return yes;
}

static void wvdl_close(struct wvdl *x, int opened) {
  if (opened) {
    wvdl_o(x, ")");
  }
}

static void wvdl_sep(struct wvdl *x, int *first) {
  if (!*first) {
    wvdl_o(x, ", ");
  }
  *first = 0;
}

static int wvdl_word(wvdl_node *n, const char *w) {
  return (n->len == (int)strlen(w)) && (memcmp(n->str, w, (size_t)n->len) == 0);
}

static void wvdl_unsupported(struct wvdl *x, wvdl_node *n, const char *what) {
  wvdl_error(x, n->pos, "%s cannot run on this engine yet", what);
}

/* ------------------------------------------------- names and keys */

static void wvdl_p_id(struct wvdl *x, wvdl_node *n) {
  if (n->binding != NULL) {
    wvdl_o(x, n->binding->out);
  } else if (wvdl_word(n, "arguments")) {
    wvdl_o(x, wvdl_args_name(x));
  } else {
    wvdl_on(x, n->str, n->len);
  }
}

static const char *wvdl_id_out(struct wvdl *x, wvdl_node *n) {
  return (n->binding != NULL) ? n->binding->out
                              : wvdl_strdup(x, n->str, n->len);
}

/* a key as a value: "name", 1, "#x", or the computed expression */
static void wvdl_p_key_value(struct wvdl *x, wvdl_node *k, int computed) {
  if (computed) {
    wvdl_p_expr(x, k, PR_ASSIGN);
  } else if (k->type == N_ID) {
    wvdl_o(x, "\"");
    wvdl_on(x, k->str, k->len);
    wvdl_o(x, "\"");
  } else if (k->type == N_PRIVATE_NAME) {
    wvdl_o(x, "\"");
    wvdl_o(x, wvdl_private(x, k));
    wvdl_o(x, "\"");
  } else {
    wvdl_on(x, k->str, k->len); /* already a literal */
  }
}

/* what follows the object: .name ["default"] [1] [expr] */
static void wvdl_p_key_access(struct wvdl *x, wvdl_node *k, int computed) {
  if (computed) {
    wvdl_o(x, "[");
    wvdl_p_expr(x, k, PR_SEQ);
    wvdl_o(x, "]");
  } else if (k->type == N_ID) {
    if (wvdl_quote_name(k->str, k->len)) {
      wvdl_o(x, "[\"");
      wvdl_on(x, k->str, k->len);
      wvdl_o(x, "\"]");
    } else {
      wvdl_o(x, ".");
      wvdl_on(x, k->str, k->len);
    }
  } else if (k->type == N_PRIVATE_NAME) {
    wvdl_o(x, ".");
    wvdl_o(x, wvdl_private(x, k));
  } else {
    wvdl_o(x, "[");
    wvdl_on(x, k->str, k->len);
    wvdl_o(x, "]");
  }
}

static void wvdl_p_key_literal(struct wvdl *x, wvdl_node *k) {
  if ((k->type == N_ID) && wvdl_quote_name(k->str, k->len)) {
    wvdl_o(x, "\"");
    wvdl_on(x, k->str, k->len);
    wvdl_o(x, "\"");
  } else {
    wvdl_on(x, k->str, k->len);
  }
}

static int wvdl_same_key(wvdl_node *a, wvdl_node *b) {
  return (a->type == b->type) && (a->len == b->len) &&
         (memcmp(a->str, b->str, (size_t)a->len) == 0);
}

/* --------------------------------------------------------- functions */

struct wvdl_fspec {
  int kind;
  const char *name;
  wvdl_node *params;
  wvdl_node *body; /* a block, or the expression of an arrow */
  int expr_body;
  int home, is_ctor;
  const char *this_preset;
  const char *pre; /* printed after the parameters are set up */
  const char *fin; /* FN_LOOP: in a finally block (copy-back) */
  int codes;       /* out: FN_LOOP exits used */
  struct wvdl_code *labels;
  int gen;         /* WVDL_F_ASYNC / WVDL_F_GEN: a state machine */
};

static void wvdl_p_genbody(struct wvdl *x, struct wvdl_fspec *f);

static void wvdl_p_decls(struct wvdl *x, struct wvdl_fn *f) {
  int any = 0;
  if (f->this_alias != NULL) {
    wvdl_o(x, " var ");
    wvdl_o(x, f->this_alias);
    wvdl_o(x, " = this");
    any = 1;
  }
  if (f->args_alias != NULL) {
    wvdl_o(x, any ? ", " : " var ");
    wvdl_o(x, f->args_alias);
    wvdl_o(x, " = arguments");
    any = 1;
  }
  if (f->vars.len > 0) {
    wvdl_o(x, any ? ", " : " var ");
    wvdl_o(x, f->vars.p);
    any = 1;
  }
  if (any) {
    wvdl_o(x, ";");
  }
}

/* the parameter list; defaults, patterns and rest become statements */
static const char *wvdl_p_params(struct wvdl *x, wvdl_node *list) {
  struct wvdl_buf names;
  wvdl_node *p;
  int index = 0;
  memset(&names, 0, sizeof(names));
  names.arena = &x->arena;
  for (p = list; p != NULL; p = p->next, index++) {
    wvdl_node *target = p, *def = NULL;
    const char *name;
    int first = 1;
    if (p->type == N_REST) {
      wvdl_o(x, " var ");
      wvdl_p_destructure(x, p->a,
                         wvdl_fmt(x, "[].slice.call(arguments, %d)", index),
                         D_DECL, &first);
      wvdl_o(x, ";");
      break;
    }
    if (p->type == N_ASSIGN_PAT) {
      target = p->a;
      def = p->b;
    }
    name = (target->type == N_ID) ? wvdl_id_out(x, target) : wvdl_fresh(x);
    if (names.len > 0) {
      wvdl_put(&names, ", ", 2);
    }
    wvdl_put(&names, name, strlen(name));
    if (def != NULL) {
      wvdl_o(x, " if (");
      wvdl_o(x, name);
      wvdl_o(x, " === void 0) ");
      wvdl_o(x, name);
      wvdl_o(x, " = ");
      wvdl_p_expr(x, def, PR_ASSIGN);
      wvdl_o(x, ";");
    }
    if (target->type != N_ID) {
      wvdl_o(x, " var ");
      wvdl_p_destructure(x, target, name, D_DECL, &first);
      wvdl_o(x, ";");
    }
  }
  return (names.p != NULL) ? names.p : "";
}

static void wvdl_p_fn(struct wvdl *x, struct wvdl_fspec *f) {
  struct wvdl_fn fn;
  struct wvdl_jump jb;
  struct wvdl_cap cap;
  const char *params, *body;
  memset(&fn, 0, sizeof(fn));
  fn.kind = f->kind;
  fn.up = x->fn;
  fn.home = f->home;
  fn.is_ctor = f->is_ctor;
  fn.this_preset = f->this_preset;
  fn.vars.arena = &x->arena;
  jb.up = x->jumps;
  jb.kind = (f->kind == FN_LOOP) ? J_IIFE : J_FUNC;
  jb.label = NULL;
  jb.len = 0;
  x->fn = &fn;
  x->jumps = &jb;
  wvdl_cap_begin(x, &cap);
  params = wvdl_p_params(x, f->params);
  if (f->pre != NULL) {
    wvdl_o(x, f->pre);
  }
  if (f->fin != NULL) {
    wvdl_o(x, " try {");
  }
  if (f->gen) {
    wvdl_p_genbody(x, f);
  } else if (f->expr_body) {
    wvdl_o(x, " return ");
    wvdl_p_expr(x, f->body, PR_ASSIGN);
    wvdl_o(x, ";");
  } else if (f->body != NULL) {
    wvdl_p_stmts(x, f->body->list);
  }
  if (f->fin != NULL) {
    wvdl_o(x, " } finally {");
    wvdl_o(x, f->fin);
    wvdl_o(x, " }");
  }
  body = wvdl_cap_end(x, &cap);
  x->fn = fn.up;
  x->jumps = jb.up;
  f->codes = fn.codes;
  f->labels = fn.labels;
  wvdl_o(x, "function ");
  if (f->name != NULL) {
    wvdl_o(x, f->name);
  }
  wvdl_o(x, "(");
  wvdl_o(x, params);
  wvdl_o(x, ") {");
  wvdl_p_decls(x, &fn);
  wvdl_o(x, body);
  wvdl_o(x, " }");
}

/* a function node; `anonymous` drops the name (hoisted block function) */
static void wvdl_p_func_node(struct wvdl *x, wvdl_node *f, int anonymous) {
  struct wvdl_fspec s;
  memset(&s, 0, sizeof(s));
  s.gen = f->flags & (WVDL_F_ASYNC | WVDL_F_GEN);
  s.kind = (f->flags & WVDL_F_ARROW) ? FN_ARROW : FN_FUNC;
  if (!anonymous && (f->a != NULL) && !(f->flags & WVDL_F_ARROW)) {
    s.name = wvdl_id_out(x, f->a);
  }
  s.params = f->list;
  s.body = f->b;
  s.expr_body = (f->flags & WVDL_F_EXPR_BODY) != 0;
  wvdl_p_fn(x, &s);
}

static void wvdl_p_method_fn(struct wvdl *x, wvdl_node *f, int home,
                             int is_ctor, const char *name, const char *pre) {
  struct wvdl_fspec s;
  memset(&s, 0, sizeof(s));
  s.gen = f->flags & (WVDL_F_ASYNC | WVDL_F_GEN);
  s.kind = FN_FUNC;
  s.name = name;
  s.params = f->list;
  s.body = f->b;
  s.home = home;
  s.is_ctor = is_ctor;
  s.pre = pre;
  wvdl_p_fn(x, &s);
}

/* ------------------------------------------------------ destructuring */

/* a temporary set to `value`: a declarator in D_DECL, else an expression */
static const char *wvdl_d_temp(struct wvdl *x, int mode, const char *value,
                               int *first) {
  const char *t = (mode == D_DECL) ? wvdl_fresh(x) : wvdl_temp(x);
  wvdl_sep(x, first);
  wvdl_o(x, t);
  wvdl_o(x, " = ");
  wvdl_o(x, value);
  return t;
}

/* "a = v.a, b = v.b": declarators (D_DECL, D_HOIST) or assignments.
   `value` is evaluated once. */
static void wvdl_p_destructure(struct wvdl *x, wvdl_node *p, const char *value,
                               int mode, int *first) {
  wvdl_node *q;
  const char *t;
  int i, count, rest;
  switch (p->type) {
  case N_ID:
    wvdl_sep(x, first);
    wvdl_p_id(x, p);
    if (mode == D_HOIST) {
      wvdl_hoist(x, wvdl_id_out(x, p));
    }
    wvdl_o(x, " = ");
    wvdl_o(x, value);
    return;
  case N_MEMBER:
    if (mode != D_ASSIGN) {
      break;
    }
    wvdl_sep(x, first);
    wvdl_p_expr(x, p, PR_CALL);
    wvdl_o(x, " = ");
    wvdl_o(x, value);
    return;
  case N_ASSIGN_PAT: {
    struct wvdl_cap c;
    const char *v =
        wvdl_is_temp(x, value) ? value : wvdl_d_temp(x, mode, value, first);
    wvdl_cap_begin(x, &c);
    wvdl_o(x, v);
    wvdl_o(x, " === void 0 ? ");
    wvdl_p_expr(x, p->b, PR_ASSIGN);
    wvdl_o(x, " : ");
    wvdl_o(x, v);
    wvdl_p_destructure(x, p->a, wvdl_cap_end(x, &c), mode, first);
    return;
  }
  case N_ARRAY_PAT:
    count = 0;
    rest = 0;
    for (q = p->list; q != NULL; q = q->next) {
      if (q->type == N_REST) {
        rest = 1;
      } else {
        count++;
      }
    }
    t = wvdl_d_temp(x, mode,
                    rest ? wvdl_fmt(x, "__wv.arr(%s)", value)
                         : wvdl_fmt(x, "__wv.arr(%s, %d)", value, count),
                    first);
    for (q = p->list, i = 0; q != NULL; q = q->next, i++) {
      if (q->type == N_HOLE) {
        continue;
      }
      if (q->type == N_REST) {
        wvdl_p_destructure(x, q->a, wvdl_fmt(x, "%s.slice(%d)", t, i), mode,
                           first);
      } else {
        wvdl_p_destructure(x, q, wvdl_fmt(x, "%s[%d]", t, i), mode, first);
      }
    }
    return;
  case N_OBJECT_PAT: {
    struct wvdl_buf keys;
    memset(&keys, 0, sizeof(keys));
    keys.arena = &x->arena;
    t = wvdl_is_temp(x, value) ? value : wvdl_d_temp(x, mode, value, first);
    for (q = p->list; q != NULL; q = q->next) {
      const char *access, *key;
      if (q->op == P_SPREAD) {
        wvdl_p_destructure(
            x, q->b,
            wvdl_fmt(x, "__wv.rest(%s, [%s])", t, keys.p ? keys.p : ""), mode,
            first);
        continue;
      }
      if (q->flags & WVDL_F_COMPUTED) {
        key = wvdl_d_temp(x, mode, wvdl_expr_str(x, q->a, PR_ASSIGN), first);
        access = wvdl_fmt(x, "%s[%s]", t, key);
      } else {
        struct wvdl_cap c;
        wvdl_cap_begin(x, &c);
        wvdl_p_key_value(x, q->a, 0);
        key = wvdl_cap_end(x, &c);
        wvdl_cap_begin(x, &c);
        wvdl_o(x, t);
        wvdl_p_key_access(x, q->a, 0);
        access = wvdl_cap_end(x, &c);
      }
      if (keys.len > 0) {
        wvdl_put(&keys, ", ", 2);
      }
      wvdl_put(&keys, key, strlen(key));
      wvdl_p_destructure(x, q->b, access, mode, first);
    }
    return;
  }
  default:
    break;
  }
  wvdl_error(x, p->pos, "invalid destructuring target");
}

/* --------------------------------------------------- object, array */

static int wvdl_has_spread(wvdl_node *l) {
  for (; l != NULL; l = l->next) {
    if (l->type == N_SPREAD) {
      return 1;
    }
  }
  return 0;
}

static void wvdl_p_element(struct wvdl *x, wvdl_node *q) {
  wvdl_line_to(x, q->pos);
  if (q->type == N_HOLE) {
    wvdl_o(x, "void 0");
  } else {
    wvdl_p_expr(x, q, PR_ASSIGN);
  }
}

/* [a, ...b, c] -> [a].concat(__wv.arr(b), [c]); `copy`: always a new array */
static void wvdl_p_spread_array(struct wvdl *x, wvdl_node *list, int copy) {
  wvdl_node *q = list;
  int first;
  if ((q != NULL) && (q->type == N_SPREAD) && (q->next == NULL) && !copy) {
    wvdl_o(x, "__wv.arr(");
    wvdl_p_expr(x, q->a, PR_ASSIGN);
    wvdl_o(x, ")");
    return;
  }
  if ((q == NULL) || (q->type == N_SPREAD)) {
    wvdl_o(x, "[]");
  } else {
    wvdl_o(x, "[");
    for (first = 1; (q != NULL) && (q->type != N_SPREAD); q = q->next) {
      wvdl_sep(x, &first);
      wvdl_p_element(x, q);
    }
    wvdl_o(x, "]");
  }
  if (q == NULL) {
    return;
  }
  wvdl_o(x, ".concat(");
  first = 1;
  while (q != NULL) {
    wvdl_sep(x, &first);
    if (q->type == N_SPREAD) {
      wvdl_line_to(x, q->pos);
      wvdl_o(x, "__wv.arr(");
      wvdl_p_expr(x, q->a, PR_ASSIGN);
      wvdl_o(x, ")");
      q = q->next;
    } else {
      int f2 = 1;
      wvdl_o(x, "[");
      for (; (q != NULL) && (q->type != N_SPREAD); q = q->next) {
        wvdl_sep(x, &f2);
        wvdl_p_element(x, q);
      }
      wvdl_o(x, "]");
    }
  }
  wvdl_o(x, ")");
}

static void wvdl_p_array(struct wvdl *x, wvdl_node *n) {
  wvdl_node *q;
  if (wvdl_has_spread(n->list)) {
    wvdl_p_spread_array(x, n->list, 1);
    return;
  }
  wvdl_o(x, "[");
  for (q = n->list; q != NULL; q = q->next) {
    if (q != n->list) {
      wvdl_o(x, ", ");
    }
    if (q->type == N_HOLE) {
      if (q->next == NULL) {
        wvdl_o(x, "void 0"); /* [1, ,] is one element, IE8 says two */
      }
    } else {
      wvdl_line_to(x, q->pos);
      wvdl_p_expr(x, q, PR_ASSIGN);
    }
  }
  wvdl_o(x, "]");
}

/* __wv.acc(obj, key, getter, setter), with the other half if it follows */
static void wvdl_p_acc(struct wvdl *x, const char *target, wvdl_node *p,
                       int home) {
  wvdl_node *get = NULL, *set = NULL, *q;
  int computed = (p->flags & WVDL_F_COMPUTED) != 0;
  if (p->op == P_GET) {
    get = p->b;
  } else {
    set = p->b;
  }
  if (!computed) {
    for (q = p->next; q != NULL; q = q->next) {
      if (!(q->extra & WVDL_X_DONE) && ((q->op == P_GET) || (q->op == P_SET)) &&
          (q->op != p->op) && (q->is_static == p->is_static) &&
          !(q->flags & WVDL_F_COMPUTED) && wvdl_same_key(q->a, p->a)) {
        if (q->op == P_GET) {
          get = q->b;
        } else {
          set = q->b;
        }
        q->extra |= WVDL_X_DONE;
        break;
      }
    }
  }
  wvdl_o(x, "__wv.acc(");
  wvdl_o(x, target);
  wvdl_o(x, ", ");
  wvdl_p_key_value(x, p->a, computed);
  wvdl_o(x, ", ");
  if (get != NULL) {
    wvdl_p_method_fn(x, get, home, 0, NULL, NULL);
  } else {
    wvdl_o(x, "void 0");
  }
  wvdl_o(x, ", ");
  if (set != NULL) {
    wvdl_p_method_fn(x, set, home, 0, NULL, NULL);
  } else {
    wvdl_o(x, "void 0");
  }
  wvdl_o(x, (home == 0) ? ", 1)" : ")"); /* object literal: enumerable */
}

static void wvdl_p_prop_value(struct wvdl *x, wvdl_node *p) {
  if (p->op == P_METHOD) {
    wvdl_p_method_fn(x, p->b, 0, 0, NULL, NULL);
  } else if (p->b->type == N_ASSIGN_PAT) {
    wvdl_error(x, p->pos, "invalid shorthand property");
  } else {
    wvdl_p_expr(x, p->b, PR_ASSIGN);
  }
}

static void wvdl_p_object_lit(struct wvdl *x, wvdl_node *n) {
  wvdl_node *p, *dyn = NULL;
  const char *t = NULL;
  int first = 1;
  for (p = n->list; p != NULL; p = p->next) {
    if ((p->flags & WVDL_F_COMPUTED) || (p->op == P_SPREAD) ||
        (p->op == P_GET) || (p->op == P_SET)) {
      dyn = p;
      break;
    }
  }
  if (dyn != NULL) {
    t = wvdl_temp(x);
    wvdl_o(x, "(");
    wvdl_o(x, t);
    wvdl_o(x, " = ");
  }
  wvdl_o(x, "{");
  for (p = n->list; p != dyn; p = p->next) {
    wvdl_sep(x, &first);
    wvdl_line_to(x, p->pos);
    wvdl_p_key_literal(x, p->a);
    wvdl_o(x, ": ");
    wvdl_p_prop_value(x, p);
  }
  wvdl_o(x, "}");
  if (dyn == NULL) {
    return;
  }
  /* from the first computed key, spread or accessor on: in order */
  for (p = dyn; p != NULL; p = p->next) {
    if (p->extra & WVDL_X_DONE) {
      continue;
    }
    wvdl_o(x, ", ");
    wvdl_line_to(x, p->pos);
    if (p->op == P_SPREAD) {
      wvdl_o(x, "__wv.assign(");
      wvdl_o(x, t);
      wvdl_o(x, ", ");
      wvdl_p_expr(x, p->b, PR_ASSIGN);
      wvdl_o(x, ")");
    } else if ((p->op == P_GET) || (p->op == P_SET)) {
      wvdl_p_acc(x, t, p, 0);
    } else {
      wvdl_o(x, t);
      wvdl_p_key_access(x, p->a, p->flags & WVDL_F_COMPUTED);
      wvdl_o(x, " = ");
      wvdl_p_prop_value(x, p);
    }
  }
  wvdl_o(x, ", ");
  wvdl_o(x, t);
  wvdl_o(x, ")");
}

/* ----------------------------------------------- templates, regexes */

static void wvdl_p_template(struct wvdl *x, wvdl_node *t, int prec) {
  wvdl_node *q = t->list;
  int opened;
  if (q->str == NULL) {
    wvdl_error(x, q->pos, "invalid escape in template literal");
  }
  if (q->next == NULL) {
    wvdl_on(x, q->str, q->len);
    return;
  }
  opened = wvdl_open(x, prec > PR_ADD);
  wvdl_on(x, q->str, q->len); /* even "": the result is a string */
  for (q = q->next; q != NULL; q = q->next) {
    if ((q->type == N_STR) && (q->a != NULL)) { /* a text chunk */
      if (q->str == NULL) {
        wvdl_error(x, q->pos, "invalid escape in template literal");
      }
      if (q->len > 2) {
        wvdl_o(x, " + ");
        wvdl_on(x, q->str, q->len);
      }
    } else {
      wvdl_o(x, " + ");
      wvdl_p_expr(x, q, PR_MUL);
    }
  }
  wvdl_close(x, opened);
}

/* tag`a${b}c` -> tag(__wv.tpl("site", ["a", "c"], ["a", "c"]), b) */
static void wvdl_p_tagged(struct wvdl *x, wvdl_node *n, int prec) {
  wvdl_node *q;
  int opened = wvdl_open(x, prec > PR_CALL), first;
  wvdl_p_expr(x, n->a, PR_CALL);
  wvdl_o(x, wvdl_fmt(x, "(__wv.tpl(\"%s:%d\", [", x->tpl_id, ++x->tpl_sites));
  for (q = n->b->list, first = 1; q != NULL; q = q->next) {
    if ((q->type == N_STR) && (q->a != NULL)) {
      wvdl_sep(x, &first);
      if (q->str != NULL) {
        wvdl_on(x, q->str, q->len);
      } else {
        wvdl_o(x, "void 0");
      }
    }
  }
  wvdl_o(x, "], [");
  for (q = n->b->list, first = 1; q != NULL; q = q->next) {
    if ((q->type == N_STR) && (q->a != NULL)) {
      wvdl_sep(x, &first);
      wvdl_on(x, q->a->str, q->a->len);
    }
  }
  wvdl_o(x, "])");
  for (q = n->b->list; q != NULL; q = q->next) {
    if (!((q->type == N_STR) && (q->a != NULL))) {
      wvdl_o(x, ", ");
      wvdl_p_expr(x, q, PR_ASSIGN);
    }
  }
  wvdl_o(x, ")");
  wvdl_close(x, opened);
}

/* a regex body as a string literal: every backslash kept */
static const char *wvdl_regex_string(struct wvdl *x, const char *s, int n) {
  struct wvdl_buf b;
  int i;
  memset(&b, 0, sizeof(b));
  b.arena = &x->arena;
  wvdl_put_char(&b, '"');
  for (i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if ((c == '\\') || (c == '"')) {
      wvdl_put_char(&b, '\\');
      wvdl_put_char(&b, (char)c);
    } else if (c >= 0x80) {
      i += wvdl_put_utf8(&b, s + i, n - i) - 1;
    } else {
      wvdl_put_char(&b, (char)c);
    }
  }
  wvdl_put_char(&b, '"');
  if (b.failed) {
    wvdl_error(x, 0, "out of memory");
  }
  return b.p;
}

struct wvdl_group {
  const char *name;
  int len;
  int index;
};

/* Regular expressions: named groups lose their names (\k<n> becomes \1),
   the s flag turns . into [\s\S], u and y are dropped, \u{...} spelled out.
   Lookbehind and \p{...} have no old form: new RegExp(...) then, so the
   script still loads and only that expression fails. */
static void wvdl_p_regex(struct wvdl *x, wvdl_node *n) {
  const char *s = n->str;
  int end = n->len - 1, i, groups = 0, ngroups = 0, in_class = 0;
  int dotall = 0, unicode = 0, risky = 0;
  struct wvdl_group named[32];
  struct wvdl_buf b, f;
  while (s[end] != '/') {
    end--;
  }
  memset(&b, 0, sizeof(b));
  memset(&f, 0, sizeof(f));
  b.arena = &x->arena;
  f.arena = &x->arena;
  for (i = end + 1; i < n->len; i++) {
    if (strchr("gim", s[i])) {
      wvdl_put_char(&f, s[i]);
    } else if (s[i] == 's') {
      dotall = 1;
    } else if (s[i] == 'u') {
      unicode = 1;
    }
  }
  /* number the groups first: \k<name> may come before its group */
  for (i = 1; i < end; i++) {
    if (s[i] == '\\') {
      i++;
    } else if (s[i] == '[') {
      in_class = 1;
    } else if (s[i] == ']') {
      in_class = 0;
    } else if ((s[i] == '(') && !in_class) {
      if (s[i + 1] != '?') {
        groups++;
      } else if ((s[i + 2] == '<') && (s[i + 3] != '=') && (s[i + 3] != '!')) {
        int j = i + 3;
        groups++;
        while ((j < end) && (s[j] != '>')) {
          j++;
        }
        if (ngroups < 32) {
          named[ngroups].name = s + i + 3;
          named[ngroups].len = j - (i + 3);
          named[ngroups].index = groups;
          ngroups++;
        }
      }
    }
  }
  in_class = 0;
  for (i = 1; i < end; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '\\') {
      char e = s[i + 1];
      if ((e == 'k') && (s[i + 2] == '<')) {
        int j = i + 3, k;
        while ((j < end) && (s[j] != '>')) {
          j++;
        }
        for (k = 0; k < ngroups; k++) {
          if ((named[k].len == j - (i + 3)) &&
              (memcmp(named[k].name, s + i + 3, (size_t)named[k].len) == 0)) {
            wvdl_puts(&b, wvdl_fmt(x, "\\%d", named[k].index));
            break;
          }
        }
        i = j;
        continue;
      }
      if (unicode && (e == 'u') && (s[i + 2] == '{')) {
        unsigned long cp = 0;
        int j = i + 3;
        while ((j < end) && (s[j] != '}')) {
          cp = cp * 16 + (unsigned long)wvdl_hex_value(s[j]);
          j++;
        }
        wvdl_put_code_point(&b, cp);
        i = j;
        continue;
      }
      if ((e == 'p') || (e == 'P')) {
        risky = 1;
      }
      if ((unsigned char)e >= 0x80) {
        i += wvdl_put_utf8(&b, s + i + 1, end - i - 1);
        continue;
      }
      wvdl_put_char(&b, '\\');
      wvdl_put_char(&b, e);
      i++;
      continue;
    }
    if (c == '[') {
      in_class = 1;
    } else if (c == ']') {
      in_class = 0;
    }
    if ((c == '(') && !in_class && (s[i + 1] == '?') && (s[i + 2] == '<')) {
      if ((s[i + 3] == '=') || (s[i + 3] == '!')) {
        risky = 1; /* lookbehind */
      } else {
        while ((i < end) && (s[i] != '>')) {
          i++;
        }
        wvdl_put_char(&b, '(');
        continue;
      }
    }
    if ((c == '.') && !in_class && dotall) {
      wvdl_puts(&b, "[\\s\\S]");
    } else if (c >= 0x80) {
      i += wvdl_put_utf8(&b, s + i, end - i) - 1;
    } else {
      wvdl_put_char(&b, (char)c);
    }
  }
  if (b.failed || f.failed) {
    wvdl_error(x, n->pos, "out of memory");
  }
  if (!risky) {
    wvdl_o(x, "/");
    wvdl_o(x, b.p ? b.p : "(?:)");
    wvdl_o(x, "/");
    wvdl_o(x, f.p ? f.p : "");
    return;
  }
  /* the runtime makes what it can of it, when (and if) it runs */
  wvdl_o(x, "__wv.re(");
  wvdl_o(x, wvdl_regex_string(x, s + 1, end - 1));
  wvdl_o(x, ", \"");
  wvdl_on(x, s + end + 1, n->len - end - 1);
  wvdl_o(x, "\")");
}

/* ------------------------------------------------ members and calls */

static void wvdl_p_super_base(struct wvdl *x, wvdl_node *n) {
  struct wvdl_fn *h = wvdl_home(x);
  if ((h == NULL) || (x->cls == NULL)) {
    wvdl_error(x, n->pos, "'super' outside of a class method");
  }
  wvdl_o(x, x->cls->super_name ? x->cls->super_name : "Object");
  if (h->home == 1) {
    wvdl_o(x, ".prototype");
  }
}

static void wvdl_p_object(struct wvdl *x, wvdl_node *o) {
  if (o->type == N_SUPER) {
    wvdl_p_super_base(x, o);
  } else if ((o->type == N_NUM) &&
             (strspn(o->str, "0123456789") == (size_t)o->len)) {
    wvdl_o(x, "(");
    wvdl_on(x, o->str, o->len);
    wvdl_o(x, ")");
  } else {
    wvdl_p_expr(x, o, PR_CALL);
  }
}

/* "k" for o.k and o["k"] */
static const char *wvdl_key_lit(struct wvdl *x, wvdl_node *m) {
  if (m->b->type == N_ID) {
    return wvdl_fmt(x, "\"%.*s\"", m->b->len, m->b->str);
  }
  return wvdl_fmt(x, "%.*s", m->b->len, m->b->str);
}

/* the object of o.k, evaluated once: o1 the first use, o2 the others */
static void wvdl_acc_obj(struct wvdl *x, wvdl_node *m, const char **o1,
                         const char **o2) {
  struct wvdl_cap c;
  const char *obj, *tmp;
  wvdl_cap_begin(x, &c);
  wvdl_p_object(x, m->a);
  obj = wvdl_cap_end(x, &c);
  if (wvdl_simple(obj)) {
    *o1 = *o2 = obj;
    return;
  }
  tmp = wvdl_temp(x);
  *o1 = wvdl_fmt(x, "(%s = %s)", tmp, obj);
  *o2 = tmp;
}

static void wvdl_p_member(struct wvdl *x, wvdl_node *n, int prec) {
  int opened, kind = wvdl_member_kind(n);
  if (kind == 1) { /* old IE: the runtime finds the getter */
    opened = wvdl_open(x, prec > PR_CALL);
    wvdl_o(x, "__wv.g(");
    wvdl_p_object(x, n->a);
    wvdl_o(x, ", ");
    wvdl_o(x, wvdl_key_lit(x, n));
    wvdl_o(x, ")");
    wvdl_close(x, opened);
    return;
  }
  if (kind == 2) { /* IE6/7: the element gets the method first */
    opened = wvdl_open(x, prec > PR_CALL);
    wvdl_o(x, "__wv.d(");
    wvdl_p_expr(x, n->a, PR_ASSIGN);
    wvdl_o(x, ")");
    wvdl_p_key_access(x, n->b, n->flags & WVDL_F_COMPUTED);
    wvdl_close(x, opened);
    return;
  }
  if (n->a->type == N_SUPER) { /* a getter up there must see this */
    opened = wvdl_open(x, prec > PR_CALL);
    wvdl_o(x, "__wv.sget(");
    wvdl_p_super_base(x, n->a);
    wvdl_o(x, ", ");
    wvdl_p_key_value(x, n->b, n->flags & WVDL_F_COMPUTED);
    wvdl_o(x, ", ");
    wvdl_o(x, wvdl_this_name(x));
    wvdl_o(x, ")");
    wvdl_close(x, opened);
    return;
  }
  opened = wvdl_open(x, prec > PR_MEMBER);
  wvdl_p_object(x, n->a);
  wvdl_p_key_access(x, n->b, n->flags & WVDL_F_COMPUTED);
  wvdl_close(x, opened);
}

/* super.x as a callee or a target: Base.prototype.x / this.x */
static void wvdl_p_super_ref(struct wvdl *x, wvdl_node *n, int target) {
  if (target) {
    wvdl_o(x, wvdl_this_name(x));
  } else {
    wvdl_p_super_base(x, n->a);
  }
  wvdl_p_key_access(x, n->b, n->flags & WVDL_F_COMPUTED);
}

static void wvdl_p_args(struct wvdl *x, wvdl_node *list) {
  wvdl_node *q;
  for (q = list; q != NULL; q = q->next) {
    if (q != list) {
      wvdl_o(x, ", ");
    }
    wvdl_line_to(x, q->pos);
    wvdl_p_expr(x, q, PR_ASSIGN);
  }
}

/* .call(self, args) or .apply(self, [..spread..]) */
static void wvdl_p_call_with(struct wvdl *x, const char *self, wvdl_node *list) {
  if (wvdl_has_spread(list)) {
    wvdl_o(x, ".apply(");
    wvdl_o(x, self);
    wvdl_o(x, ", ");
    wvdl_p_spread_array(x, list, 0);
    wvdl_o(x, ")");
  } else {
    wvdl_o(x, ".call(");
    wvdl_o(x, self);
    if (list != NULL) {
      wvdl_o(x, ", ");
      wvdl_p_args(x, list);
    }
    wvdl_o(x, ")");
  }
}

/* RegExp(p, flags) with the global RegExp: flags old engines refuse */
static int wvdl_global_regexp(wvdl_node *c, wvdl_node *args) {
  return (c->type == N_ID) && (c->binding == NULL) && wvdl_word(c, "RegExp") &&
         (args != NULL) && !wvdl_has_spread(args);
}

static void wvdl_p_call(struct wvdl *x, wvdl_node *n, int prec) {
  wvdl_node *c = n->a;
  int opened = wvdl_open(x, prec > PR_CALL);
  if (wvdl_global_regexp(c, n->list)) {
    wvdl_o(x, "__wv.re(");
    wvdl_p_args(x, n->list);
    wvdl_o(x, ")");
  } else if (c->type == N_SUPER) {
    /* super(...) in a derived constructor, then the fields */
    struct wvdl_fn *h = wvdl_home(x);
    const char *self = wvdl_this_name(x);
    if ((h == NULL) || !h->is_ctor || (x->cls == NULL) ||
        (x->cls->super_name == NULL)) {
      wvdl_error(x, c->pos, "'super()' outside of a derived constructor");
    }
    if (x->cls->init_name != NULL) {
      wvdl_o(x, "(");
    }
    wvdl_o(x, x->cls->super_name);
    wvdl_p_call_with(x, self, n->list);
    if (x->cls->init_name != NULL) {
      wvdl_o(x, ", ");
      wvdl_o(x, x->cls->init_name);
      wvdl_o(x, ".call(");
      wvdl_o(x, self);
      wvdl_o(x, "))");
    }
  } else if ((c->type == N_MEMBER) && (c->a->type == N_SUPER)) {
    wvdl_p_super_ref(x, c, 0);
    wvdl_p_call_with(x, wvdl_this_name(x), n->list);
  } else if ((c->type == N_MEMBER) && (wvdl_member_kind(c) == 1)) {
    /* old IE: __wv.g(o, "k").call(o, ...), so `this` is still o */
    const char *o1, *o2;
    wvdl_acc_obj(x, c, &o1, &o2);
    wvdl_o(x, "__wv.g(");
    wvdl_o(x, o1);
    wvdl_o(x, ", ");
    wvdl_o(x, wvdl_key_lit(x, c));
    wvdl_o(x, ")");
    wvdl_p_call_with(x, o2, n->list);
  } else if (wvdl_has_spread(n->list)) {
    if (c->type == N_MEMBER) {
      const char *obj;
      int dom = (wvdl_member_kind(c) == 2);
      if (dom) {
        wvdl_o(x, "__wv.d(");
      }
      if ((c->a->type == N_ID) || (c->a->type == N_THIS)) {
        obj = wvdl_expr_str(x, c->a, PR_CALL);
        wvdl_o(x, obj);
      } else {
        obj = wvdl_temp(x);
        wvdl_o(x, "(");
        wvdl_o(x, obj);
        wvdl_o(x, " = ");
        wvdl_p_expr(x, c->a, PR_ASSIGN);
        wvdl_o(x, ")");
      }
      if (dom) {
        wvdl_o(x, ")");
      }
      wvdl_p_key_access(x, c->b, c->flags & WVDL_F_COMPUTED);
      wvdl_p_call_with(x, obj, n->list);
    } else {
      wvdl_p_expr(x, c, PR_CALL);
      wvdl_p_call_with(x, "void 0", n->list);
    }
  } else {
    wvdl_p_expr(x, c, PR_CALL);
    wvdl_o(x, "(");
    wvdl_p_args(x, n->list);
    wvdl_o(x, ")");
  }
  wvdl_close(x, opened);
}

static int wvdl_spine_call(wvdl_node *c) {
  while (c->type == N_MEMBER) {
    c = c->a;
  }
  return (c->type == N_CALL) || (c->type == N_TAGGED) || (c->type == N_CHAIN);
}

static void wvdl_p_new(struct wvdl *x, wvdl_node *n, int prec) {
  int opened;
  if (wvdl_global_regexp(n->a, n->list)) {
    opened = wvdl_open(x, prec > PR_CALL);
    wvdl_o(x, "__wv.re(");
    wvdl_p_args(x, n->list);
    wvdl_o(x, ")");
    wvdl_close(x, opened);
    return;
  }
  if (wvdl_has_spread(n->list)) {
    opened = wvdl_open(x, prec > PR_CALL);
    wvdl_o(x, "__wv.construct(");
    wvdl_p_expr(x, n->a, PR_ASSIGN);
    wvdl_o(x, ", ");
    wvdl_p_spread_array(x, n->list, 0);
    wvdl_o(x, ")");
    wvdl_close(x, opened);
    return;
  }
  opened = wvdl_open(x, prec > PR_MEMBER);
  wvdl_o(x, "new ");
  if (wvdl_spine_call(n->a)) {
    wvdl_o(x, "(");
    wvdl_p_expr(x, n->a, PR_SEQ);
    wvdl_o(x, ")");
  } else {
    wvdl_p_expr(x, n->a, PR_MEMBER);
  }
  wvdl_o(x, "(");
  wvdl_p_args(x, n->list);
  wvdl_o(x, ")");
  wvdl_close(x, opened);
}

static const char *wvdl_cat(struct wvdl *x, const char *a, const char *b) {
  return wvdl_fmt(x, "%s%s", a, b);
}

/* a?.b.c(d)?.[e]: each ?. tests what is left of it, once */
static void wvdl_p_chain(struct wvdl *x, wvdl_node *chain, int prec) {
  wvdl_node *spine[256], *m = chain->a;
  struct wvdl_buf pre;
  struct wvdl_cap c;
  const char *cur, *cur_obj = NULL;
  int n = 0, i, opened, this_call = 0;
  memset(&pre, 0, sizeof(pre));
  pre.arena = &x->arena;
  while (((m->type == N_MEMBER) || (m->type == N_CALL)) && (n < 256)) {
    spine[n++] = m;
    m = m->a;
  }
  wvdl_cap_begin(x, &c);
  wvdl_p_object(x, m);
  cur = wvdl_cap_end(x, &c);
  for (i = n - 1; i >= 0; i--) {
    wvdl_node *s = spine[i];
    if (s->flags & WVDL_F_OPTIONAL) {
      const char *t = wvdl_temp(x);
      wvdl_puts(&pre, wvdl_fmt(x, "(%s = %s) == null ? void 0 : ", t, cur));
      cur = t;
    }
    if (s->type == N_MEMBER) {
      int kind = wvdl_member_kind(s);
      int keep = (i > 0) && (spine[i - 1]->type == N_CALL) &&
                 ((spine[i - 1]->flags & WVDL_F_OPTIONAL) ||
                  wvdl_has_spread(spine[i - 1]->list) || (kind == 1));
      wvdl_cap_begin(x, &c);
      wvdl_p_key_access(x, s->b, s->flags & WVDL_F_COMPUTED);
      if (kind != 0) { /* old IE: __wv.g(o, "k") or __wv.d(o).k */
        const char *access = wvdl_cap_end(x, &c), *base = cur;
        cur_obj = cur;
        if (keep && !wvdl_simple(cur)) {
          cur_obj = wvdl_temp(x);
          base = wvdl_fmt(x, "(%s = %s)", cur_obj, cur);
        }
        cur = (kind == 1) ?
              wvdl_fmt(x, "__wv.g(%s, %s)", base, wvdl_key_lit(x, s)) :
              wvdl_fmt(x, "__wv.d(%s)%s", base, access);
        this_call = (kind == 1);
        continue;
      }
      this_call = 0;
      if (keep && !wvdl_simple(cur)) { /* the call needs it as `this` */
        const char *t = wvdl_temp(x);
        const char *access = wvdl_cap_end(x, &c);
        cur_obj = t;
        cur = wvdl_fmt(x, "(%s = %s)%s", t, cur, access);
      } else {
        cur_obj = cur;
        cur = wvdl_cat(x, cur, wvdl_cap_end(x, &c));
      }
    } else {
      wvdl_cap_begin(x, &c);
      if ((s->flags & WVDL_F_OPTIONAL) || wvdl_has_spread(s->list) ||
          this_call) {
        wvdl_p_call_with(x, cur_obj ? cur_obj : "void 0", s->list);
      } else {
        wvdl_o(x, "(");
        wvdl_p_args(x, s->list);
        wvdl_o(x, ")");
      }
      cur = wvdl_cat(x, cur, wvdl_cap_end(x, &c));
      cur_obj = NULL;
      this_call = 0;
    }
  }
  if (pre.failed) {
    wvdl_error(x, chain->pos, "out of memory");
  }
  opened = wvdl_open(x, (pre.len > 0) && (prec > PR_COND));
  if (pre.len > 0) {
    wvdl_o(x, pre.p);
  }
  wvdl_o(x, cur);
  wvdl_close(x, opened);
}

/* ---------------------------------------------------- operators */

static int wvdl_binop_prec(wvdl_node *n) {
  const char *op;
  if (n->op < 0) {
    return PR_REL; /* in, instanceof */
  }
  op = wvdl_op_text(n->op);
  if (!strcmp(op, "||")) return PR_OR;
  if (!strcmp(op, "&&")) return PR_AND;
  if (!strcmp(op, "|")) return PR_BOR;
  if (!strcmp(op, "^")) return PR_BXOR;
  if (!strcmp(op, "&")) return PR_BAND;
  if ((op[0] == '=') || (op[0] == '!')) return PR_EQ;
  if (!strcmp(op, "<<") || !strcmp(op, ">>") || !strcmp(op, ">>>")) {
    return PR_SHIFT;
  }
  if ((op[0] == '<') || (op[0] == '>')) return PR_REL;
  if ((op[0] == '+') || (op[0] == '-')) return PR_ADD;
  return PR_MUL;
}

static void wvdl_p_binary(struct wvdl *x, wvdl_node *n, int prec) {
  int p, opened;
  if (n->op >= 0) {
    const char *op = wvdl_op_text(n->op);
    if (!strcmp(op, "**")) {
      opened = wvdl_open(x, prec > PR_CALL);
      wvdl_o(x, "Math.pow(");
      wvdl_p_expr(x, n->a, PR_ASSIGN);
      wvdl_o(x, ", ");
      wvdl_p_expr(x, n->b, PR_ASSIGN);
      wvdl_o(x, ")");
      wvdl_close(x, opened);
      return;
    }
    if (!strcmp(op, "??")) {
      opened = wvdl_open(x, prec > PR_COND);
      if ((n->a->type == N_ID) || (n->a->type == N_THIS)) {
        const char *v = wvdl_expr_str(x, n->a, PR_CALL);
        wvdl_o(x, v);
        wvdl_o(x, " != null ? ");
        wvdl_o(x, v);
      } else {
        const char *t = wvdl_temp(x);
        wvdl_o(x, "(");
        wvdl_o(x, t);
        wvdl_o(x, " = ");
        wvdl_p_expr(x, n->a, PR_ASSIGN);
        wvdl_o(x, ") != null ? ");
        wvdl_o(x, t);
      }
      wvdl_o(x, " : ");
      wvdl_p_expr(x, n->b, PR_ASSIGN);
      wvdl_close(x, opened);
      return;
    }
  }
  p = wvdl_binop_prec(n);
  opened = wvdl_open(x, (prec > p) ||
                            (x->no_in && (n->op < 0) && (n->str[0] == 'i') &&
                             (n->len == 2)));
  if ((n->op < 0) && (n->a->type == N_PRIVATE_NAME)) {
    wvdl_o(x, "\"");
    wvdl_o(x, wvdl_private(x, n->a));
    wvdl_o(x, "\"");
  } else {
    wvdl_p_expr(x, n->a, p);
  }
  wvdl_o(x, " ");
  if (n->op < 0) {
    wvdl_on(x, n->str, n->len);
  } else {
    wvdl_o(x, wvdl_op_text(n->op));
  }
  wvdl_o(x, " ");
  wvdl_p_expr(x, n->b, p + 1);
  wvdl_close(x, opened);
}

/* a reference read and then written: the first use sets the temporaries,
   the second reuses them, so obj and key are evaluated once */
static void wvdl_ref_split(struct wvdl *x, wvdl_node *t, const char **first,
                           const char **second) {
  struct wvdl_cap c;
  const char *obj, *obj1, *k1, *k2;
  if (t->type == N_ID) {
    *first = *second = wvdl_expr_str(x, t, PR_CALL);
    return;
  }
  if (t->type != N_MEMBER) {
    wvdl_error(x, t->pos, "invalid assignment target");
  }
  wvdl_cap_begin(x, &c);
  wvdl_p_object(x, t->a);
  obj = wvdl_cap_end(x, &c);
  if (wvdl_simple(obj) || (t->a->type == N_SUPER)) {
    obj1 = obj;
  } else {
    const char *tmp = wvdl_temp(x);
    obj1 = wvdl_fmt(x, "(%s = %s)", tmp, obj);
    obj = tmp;
  }
  if ((t->flags & WVDL_F_COMPUTED) && (t->b->type != N_STR) &&
      (t->b->type != N_NUM)) {
    const char *kt = wvdl_temp(x);
    k1 = wvdl_fmt(x, "[%s = %s]", kt, wvdl_expr_str(x, t->b, PR_ASSIGN));
    k2 = wvdl_fmt(x, "[%s]", kt);
  } else {
    wvdl_cap_begin(x, &c);
    wvdl_p_key_access(x, t->b, t->flags & WVDL_F_COMPUTED);
    k1 = k2 = wvdl_cap_end(x, &c);
  }
  *first = wvdl_cat(x, obj1, k1);
  *second = wvdl_cat(x, obj, k2);
}

/* old IE: o.k = v and the like through __wv.g/s */
static void wvdl_p_acc_assign(struct wvdl *x, wvdl_node *n, const char *op,
                              int prec) {
  const char *o1, *o2, *k = wvdl_key_lit(x, n->a);
  int opened;
  if (!strcmp(op, "=")) { /* the object is used once: no temporary */
    wvdl_o(x, "__wv.s(");
    wvdl_p_object(x, n->a->a);
    wvdl_o(x, wvdl_fmt(x, ", %s, ", k));
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_o(x, ")");
    return;
  }
  wvdl_acc_obj(x, n->a, &o1, &o2);
  if (!strcmp(op, "||=") || !strcmp(op, "&&=")) {
    opened = wvdl_open(x, prec > ((op[0] == '|') ? PR_OR : PR_AND));
    wvdl_o(x, wvdl_fmt(x, "__wv.g(%s, %s) %s __wv.s(%s, %s, ", o1, k,
                       (op[0] == '|') ? "||" : "&&", o2, k));
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_o(x, ")");
    wvdl_close(x, opened);
    return;
  }
  if (!strcmp(op, "?\?=")) {
    const char *t = wvdl_temp(x);
    opened = wvdl_open(x, prec > PR_COND);
    wvdl_o(x, wvdl_fmt(x, "(%s = __wv.g(%s, %s)) != null ? %s : __wv.s(%s, %s, ",
                       t, o1, k, t, o2, k));
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_o(x, ")");
    wvdl_close(x, opened);
    return;
  }
  wvdl_o(x, wvdl_fmt(x, "__wv.s(%s, %s, ", o1, k));
  if (!strcmp(op, "**=")) {
    wvdl_o(x, wvdl_fmt(x, "Math.pow(__wv.g(%s, %s), ", o2, k));
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_o(x, ")");
  } else { /* += -= *= ... : the old value, the operator, the new part */
    wvdl_o(x, wvdl_fmt(x, "__wv.g(%s, %s) %.*s (", o2, k,
                       (int)strlen(op) - 1, op));
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_o(x, ")");
  }
  wvdl_o(x, ")");
}

static void wvdl_p_assign(struct wvdl *x, wvdl_node *n, int prec) {
  const char *op = wvdl_op_text(n->op);
  int opened;
  if ((n->a->type == N_MEMBER) && (wvdl_member_kind(n->a) == 1)) {
    wvdl_p_acc_assign(x, n, op, prec);
    return;
  }
  if (!strcmp(op, "=") &&
      ((n->a->type == N_ARRAY_PAT) || (n->a->type == N_OBJECT_PAT))) {
    /* ($t = value, a = $t[0], ..., $t) */
    const char *t = wvdl_temp(x);
    int first = 0;
    wvdl_o(x, "(");
    wvdl_o(x, t);
    wvdl_o(x, " = ");
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_p_destructure(x, n->a, t, D_ASSIGN, &first);
    wvdl_o(x, ", ");
    wvdl_o(x, t);
    wvdl_o(x, ")");
    return;
  }
  if (!strcmp(op, "**=") || !strcmp(op, "||=") || !strcmp(op, "&&=") ||
      !strcmp(op, "?\?=")) {
    const char *r1, *r2;
    wvdl_ref_split(x, n->a, &r1, &r2);
    if (op[0] == '*') {
      opened = wvdl_open(x, prec > PR_ASSIGN);
      wvdl_o(x, r1);
      wvdl_o(x, " = Math.pow(");
      wvdl_o(x, r2);
      wvdl_o(x, ", ");
      wvdl_p_expr(x, n->b, PR_ASSIGN);
      wvdl_o(x, ")");
    } else if (op[0] == '?') {
      opened = wvdl_open(x, prec > PR_COND);
      if (n->a->type == N_ID) {
        wvdl_o(x, r1);
        wvdl_o(x, " != null ? ");
        wvdl_o(x, r2);
      } else {
        const char *t = wvdl_temp(x);
        wvdl_o(x, "(");
        wvdl_o(x, t);
        wvdl_o(x, " = ");
        wvdl_o(x, r1);
        wvdl_o(x, ") != null ? ");
        wvdl_o(x, t);
      }
      wvdl_o(x, " : (");
      wvdl_o(x, r2);
      wvdl_o(x, " = ");
      wvdl_p_expr(x, n->b, PR_ASSIGN);
      wvdl_o(x, ")");
    } else {
      opened = wvdl_open(x, prec > ((op[0] == '|') ? PR_OR : PR_AND));
      wvdl_o(x, r1);
      wvdl_o(x, (op[0] == '|') ? " || (" : " && (");
      wvdl_o(x, r2);
      wvdl_o(x, " = ");
      wvdl_p_expr(x, n->b, PR_ASSIGN);
      wvdl_o(x, ")");
    }
    wvdl_close(x, opened);
    return;
  }
  opened = wvdl_open(x, prec > PR_ASSIGN);
  if ((n->a->type != N_ID) && (n->a->type != N_MEMBER)) {
    wvdl_error(x, n->a->pos, "invalid assignment target");
  }
  if ((n->a->type == N_MEMBER) && (n->a->a->type == N_SUPER)) {
    wvdl_p_super_ref(x, n->a, 1);
  } else {
    wvdl_p_expr(x, n->a, PR_CALL);
  }
  wvdl_o(x, " ");
  wvdl_o(x, op);
  wvdl_o(x, " ");
  wvdl_p_expr(x, n->b, PR_ASSIGN);
  wvdl_close(x, opened);
}

static void wvdl_p_expr(struct wvdl *x, wvdl_node *n, int prec) {
  wvdl_node *q;
  int opened;
  switch (n->type) {
  case N_ID:
    wvdl_p_id(x, n);
    return;
  case N_NUM:
  case N_LIT:
    wvdl_on(x, n->str, n->len);
    return;
  case N_STR:
    if (n->str == NULL) {
      wvdl_error(x, n->pos, "invalid escape");
    }
    wvdl_on(x, n->str, n->len);
    return;
  case N_REGEX:
    wvdl_p_regex(x, n);
    return;
  case N_TEMPLATE:
    wvdl_p_template(x, n, prec);
    return;
  case N_TAGGED:
    wvdl_p_tagged(x, n, prec);
    return;
  case N_THIS:
    wvdl_o(x, wvdl_this_name(x));
    return;
  case N_ARRAY:
    wvdl_p_array(x, n);
    return;
  case N_OBJECT:
    wvdl_p_object_lit(x, n);
    return;
  case N_FUNC:
    wvdl_p_func_node(x, n, 0);
    return;
  case N_CLASS:
    wvdl_p_class(x, n, NULL);
    return;
  case N_UNARY:
    opened = wvdl_open(x, prec > PR_UNARY);
    if (n->op < 0) { /* typeof void delete */
      wvdl_on(x, n->str, n->len);
      wvdl_o(x, " ");
      wvdl_p_expr(x, n->a, PR_UNARY);
    } else {
      const char *op = wvdl_op_text(n->op);
      const char *v = wvdl_expr_str(x, n->a, PR_UNARY);
      wvdl_o(x, op);
      if (((op[0] == '+') || (op[0] == '-')) && (v[0] == op[0])) {
        wvdl_o(x, " "); /* - -x, not --x */
      }
      wvdl_o(x, v);
    }
    wvdl_close(x, opened);
    return;
  case N_UPDATE:
    if ((n->a->type == N_MEMBER) && (wvdl_member_kind(n->a) == 1)) {
      /* old IE: __wv.u(o, "k", +1 or -1, prefix) */
      wvdl_o(x, "__wv.u(");
      wvdl_p_object(x, n->a->a);
      wvdl_o(x, ", ");
      wvdl_o(x, wvdl_key_lit(x, n->a));
      wvdl_o(x, (wvdl_op_text(n->op)[0] == '+') ? ", 1, " : ", -1, ");
      wvdl_o(x, n->flags ? "1)" : "0)");
      return;
    }
    opened = wvdl_open(x, prec > (n->flags ? PR_UNARY : PR_POSTFIX));
    if (n->flags) {
      wvdl_o(x, wvdl_op_text(n->op));
      wvdl_p_expr(x, n->a, PR_UNARY);
    } else {
      wvdl_p_expr(x, n->a, PR_POSTFIX);
      wvdl_o(x, wvdl_op_text(n->op));
    }
    wvdl_close(x, opened);
    return;
  case N_BINARY:
    wvdl_p_binary(x, n, prec);
    return;
  case N_ASSIGN:
    wvdl_p_assign(x, n, prec);
    return;
  case N_COND:
    opened = wvdl_open(x, prec > PR_COND);
    wvdl_p_expr(x, n->a, PR_OR);
    wvdl_o(x, " ? ");
    wvdl_p_expr(x, n->b, PR_ASSIGN);
    wvdl_o(x, " : ");
    wvdl_p_expr(x, n->c, PR_ASSIGN);
    wvdl_close(x, opened);
    return;
  case N_CALL:
    wvdl_p_call(x, n, prec);
    return;
  case N_NEW:
    wvdl_p_new(x, n, prec);
    return;
  case N_MEMBER:
    wvdl_p_member(x, n, prec);
    return;
  case N_SEQ:
    opened = wvdl_open(x, prec > PR_SEQ);
    for (q = n->list; q != NULL; q = q->next) {
      if (q != n->list) {
        wvdl_o(x, ", ");
      }
      wvdl_p_expr(x, q, PR_ASSIGN);
    }
    wvdl_close(x, opened);
    return;
  case N_CHAIN:
    wvdl_p_chain(x, n, prec);
    return;
  case N_PRIVATE_NAME:
    wvdl_o(x, "\"");
    wvdl_o(x, wvdl_private(x, n));
    wvdl_o(x, "\"");
    return;
  case N_YIELD:
    wvdl_unsupported(x, n, "generators");
    return;
  case N_AWAIT:
    wvdl_unsupported(x, n, "async functions");
    return;
  case N_NEW_TARGET:
    wvdl_unsupported(x, n, "new.target");
    return;
  case N_RAW:
    wvdl_o(x, n->str);
    return;
  default:
    break;
  }
  wvdl_error(x, n->pos, "unexpected expression");
}

/* ------------------------------------------------------------ classes */

static void wvdl_p_field(struct wvdl *x, wvdl_node *m, const char *self) {
  if (m->a->type == N_PRIVATE_NAME) {
    wvdl_o(x, "__wv.p(");
    wvdl_o(x, self);
    wvdl_o(x, ", \"");
    wvdl_o(x, wvdl_private(x, m->a));
    wvdl_o(x, "\", ");
  } else {
    wvdl_o(x, self);
    wvdl_p_key_access(x, m->a, m->flags & WVDL_F_COMPUTED);
    wvdl_o(x, " = ");
  }
  if (m->b != NULL) {
    wvdl_p_expr(x, m->b, PR_ASSIGN);
  } else {
    wvdl_o(x, "void 0");
  }
  wvdl_o(x, (m->a->type == N_PRIVATE_NAME) ? ");" : ";");
}

/*
 * class A extends B { ... }  ->
 *
 *   (function ($s) {
 *     __wv.inherits(A, $s);
 *     function A(...) { ... }             fields: $init.call(this)
 *     __wv.m(A.prototype, "m", function () { ... $s.prototype.m.call(this) });
 *     __wv.acc(A.prototype, "x", getter, setter);
 *     function $init() { this.f = 1; __wv.p(this, "$wv_1_p", 2); }
 *     return A;
 *   }(__wv.base(B)))
 */
static void wvdl_p_class(struct wvdl *x, wvdl_node *c, const char *hint) {
  struct wvdl_cls cls;
  struct wvdl_fn fn;
  struct wvdl_cap cap;
  const char *heritage = NULL, *body, *init_text = NULL, *proto;
  wvdl_node *m, *ctor = NULL;
  int has_fields = 0;
  if (c->b != NULL) {
    heritage = wvdl_expr_str(x, c->b, PR_ASSIGN);
  }
  memset(&cls, 0, sizeof(cls));
  cls.up = x->cls;
  cls.node = c;
  cls.id = ++x->cls_counter;
  cls.name = (c->a != NULL) ? wvdl_strdup(x, c->a->str, c->a->len)
             : (hint != NULL) ? hint
                              : wvdl_fresh(x);
  for (m = c->list; m != NULL; m = m->next) {
    if (m->op == P_CTOR) {
      ctor = m;
    } else if ((m->op == P_FIELD) && !m->is_static) {
      has_fields = 1;
    }
  }
  if (c->b != NULL) {
    cls.super_name = wvdl_fresh(x);
  }
  if (has_fields) {
    cls.init_name = wvdl_fresh(x);
  }
  proto = wvdl_fmt(x, "%s.prototype", cls.name);
  memset(&fn, 0, sizeof(fn));
  fn.kind = FN_ARROW; /* computed keys see the outer this */
  fn.up = x->fn;
  fn.vars.arena = &x->arena;
  x->fn = &fn;
  x->cls = &cls;
  if (has_fields) {
    struct wvdl_fn ifn;
    struct wvdl_cap fc, dc;
    const char *fields;
    memset(&ifn, 0, sizeof(ifn));
    ifn.kind = FN_FUNC;
    ifn.home = 1;
    ifn.up = x->fn;
    ifn.vars.arena = &x->arena;
    x->fn = &ifn;
    wvdl_cap_begin(x, &fc);
    for (m = c->list; m != NULL; m = m->next) {
      if ((m->op == P_FIELD) && !m->is_static) {
        wvdl_stmt_start(x, m->pos);
        wvdl_p_field(x, m, "this");
      }
    }
    fields = wvdl_cap_end(x, &fc);
    x->fn = ifn.up;
    wvdl_cap_begin(x, &dc);
    wvdl_p_decls(x, &ifn);
    init_text = wvdl_fmt(x, " function %s() {%s%s }", cls.init_name,
                         wvdl_cap_end(x, &dc), fields);
  }
  wvdl_cap_begin(x, &cap);
  if (c->b != NULL) {
    wvdl_o(x, wvdl_fmt(x, " __wv.inherits(%s, %s);", cls.name, cls.super_name));
  }
  if (ctor == NULL) {
    wvdl_o(x, wvdl_fmt(x, " function %s() {", cls.name));
    if (cls.super_name != NULL) {
      wvdl_o(x, wvdl_fmt(x, " %s.apply(this, arguments);", cls.super_name));
    }
    if (cls.init_name != NULL) {
      wvdl_o(x, wvdl_fmt(x, " %s.call(this);", cls.init_name));
    }
    wvdl_o(x, " }");
  }
  for (m = c->list; m != NULL; m = m->next) {
    const char *target = m->is_static ? cls.name : proto;
    if (m->extra & WVDL_X_DONE) {
      continue;
    }
    switch (m->op) {
    case P_CTOR:
      wvdl_stmt_start(x, m->pos);
      wvdl_p_method_fn(x, m->b, 1, 1, cls.name,
                       ((cls.init_name != NULL) && (cls.super_name == NULL))
                           ? wvdl_fmt(x, " %s.call(this);", cls.init_name)
                           : NULL);
      break;
    case P_METHOD:
      wvdl_stmt_start(x, m->pos);
      wvdl_o(x, "__wv.m(");
      wvdl_o(x, target);
      wvdl_o(x, ", ");
      wvdl_p_key_value(x, m->a, m->flags & WVDL_F_COMPUTED);
      wvdl_o(x, ", ");
      wvdl_p_method_fn(x, m->b, m->is_static ? 2 : 1, 0, NULL, NULL);
      wvdl_o(x, ");");
      break;
    case P_GET:
    case P_SET:
      wvdl_stmt_start(x, m->pos);
      wvdl_p_acc(x, target, m, m->is_static ? 2 : 1);
      wvdl_o(x, ";");
      break;
    case P_FIELD:
      if (m->is_static) {
        struct wvdl_fn tf;
        memset(&tf, 0, sizeof(tf));
        tf.kind = FN_THIS;
        tf.this_preset = cls.name;
        tf.home = 2;
        tf.up = x->fn;
        x->fn = &tf;
        wvdl_stmt_start(x, m->pos);
        wvdl_p_field(x, m, cls.name);
        x->fn = tf.up;
      }
      break;
    case P_STATIC_BLOCK: {
      struct wvdl_fspec s;
      memset(&s, 0, sizeof(s));
      s.kind = FN_FUNC;
      s.body = m->b;
      s.home = 2;
      wvdl_stmt_start(x, m->pos);
      wvdl_o(x, "(");
      wvdl_p_fn(x, &s);
      wvdl_o(x, wvdl_fmt(x, ").call(%s);", cls.name));
      break;
    }
    default:
      break;
    }
  }
  if (init_text != NULL) {
    wvdl_o(x, init_text);
  }
  wvdl_o(x, wvdl_fmt(x, " return %s;", cls.name));
  body = wvdl_cap_end(x, &cap);
  x->cls = cls.up;
  x->fn = fn.up;
  wvdl_o(x, "(function (");
  if (cls.super_name != NULL) {
    wvdl_o(x, cls.super_name);
  }
  wvdl_o(x, ") {");
  wvdl_p_decls(x, &fn);
  wvdl_o(x, body);
  wvdl_o(x, " }(");
  if (heritage != NULL) {
    wvdl_o(x, "__wv.base(");
    wvdl_o(x, heritage);
    wvdl_o(x, ")");
  }
  wvdl_o(x, "))");
}

/* --------------------------------------------------------- statements */

static int wvdl_in_iife(struct wvdl *x) {
  return x->fn->kind == FN_LOOP;
}

/* declarations become assignments to variables of an outer function:
   var in a loop body run as a function, everything in a state machine */
static int wvdl_hoisting(struct wvdl *x, int kind) {
  return (x->fn->kind == FN_GEN) || ((kind == K_VAR) && wvdl_in_iife(x));
}

/* var / let / const; returns 1 when nothing was printed (hoisted) */
static int wvdl_p_var(struct wvdl *x, wvdl_node *v, int head) {
  wvdl_node *d;
  int first = 1;
  int hoist = wvdl_hoisting(x, v->op);
  if (!hoist) {
    wvdl_o(x, "var ");
  }
  for (d = v->list; d != NULL; d = d->next) {
    if (d->a->type != N_ID) {
      if (d->b == NULL) {
        wvdl_error(x, d->pos, "destructuring needs a value");
      }
      wvdl_line_to(x, d->pos);
      wvdl_p_destructure(x, d->a, wvdl_expr_str(x, d->b, PR_ASSIGN),
                         hoist ? D_HOIST : D_DECL, &first);
      continue;
    }
    if (hoist) {
      wvdl_hoist(x, wvdl_id_out(x, d->a));
      if ((d->b == NULL) &&
          ((v->op == K_VAR) || head || (d->a->binding == NULL) ||
           d->a->binding->scope->is_fn)) {
        continue;
      }
    }
    wvdl_sep(x, &first);
    wvdl_line_to(x, d->pos);
    wvdl_p_id(x, d->a);
    if (d->b != NULL) {
      wvdl_o(x, " = ");
      if ((d->b->type == N_CLASS) && (d->b->a == NULL)) {
        wvdl_p_class(x, d->b, wvdl_strdup(x, d->a->str, d->a->len));
      } else {
        wvdl_p_expr(x, d->b, PR_ASSIGN);
      }
    } else if ((v->op != K_VAR) && !head && (d->a->binding != NULL) &&
               !d->a->binding->scope->is_fn) {
      wvdl_o(x, " = void 0"); /* a fresh let each time round a loop */
    }
  }
  return first;
}

static int wvdl_iife_owns(struct wvdl_jump *j, const char *label, int len) {
  struct wvdl_jump *k = j->up;
  if ((k == NULL) || (k->kind != J_LOOP)) {
    return 0;
  }
  for (k = k->up; (k != NULL) && (k->kind == J_LABEL); k = k->up) {
    if ((k->len == len) && (memcmp(k->label, label, (size_t)len) == 0)) {
      return 1;
    }
  }
  return 0;
}

/* break / continue; inside a loop body run as a function they return a
   code the loop acts on */
static void wvdl_p_jump(struct wvdl *x, wvdl_node *s) {
  int brk = (s->type == N_BREAK);
  struct wvdl_jump *j;
  for (j = x->jumps; j != NULL; j = j->up) {
    if (j->kind == J_FUNC) {
      break;
    }
    if (j->kind == J_IIFE) {
      if ((s->str == NULL) || wvdl_iife_owns(j, s->str, s->len)) {
        if (brk) {
          x->fn->codes |= 1;
          wvdl_o(x, "return \"b\";");
        } else {
          wvdl_o(x, "return;");
        }
      } else {
        const char *text =
            wvdl_fmt(x, "%c:%.*s", brk ? 'b' : 'c', s->len, s->str);
        struct wvdl_code *c;
        for (c = x->fn->labels; c != NULL; c = c->next) {
          if (!strcmp(c->text, text)) {
            break;
          }
        }
        if (c == NULL) {
          c = (struct wvdl_code *)wvdl_alloc(&x->arena, sizeof(*c));
          if (c == NULL) {
            wvdl_error(x, s->pos, "out of memory");
          }
          c->text = text;
          c->label = s->str;
          c->len = s->len;
          c->is_break = brk;
          c->next = x->fn->labels;
          x->fn->labels = c;
        }
        wvdl_o(x, "return \"");
        wvdl_o(x, text);
        wvdl_o(x, "\";");
      }
      return;
    }
    if (s->str == NULL) {
      if ((j->kind == J_LOOP) || (brk && (j->kind == J_SWITCH))) {
        break;
      }
    } else if ((j->kind == J_LABEL) && (j->len == s->len) &&
               (memcmp(j->label, s->str, (size_t)s->len) == 0)) {
      break;
    }
  }
  wvdl_o(x, brk ? "break" : "continue");
  if (s->str != NULL) {
    wvdl_o(x, " ");
    wvdl_on(x, s->str, s->len);
  }
  wvdl_o(x, ";");
}

static void wvdl_p_return(struct wvdl *x, wvdl_node *s) {
  if (x->fn->kind == FN_GEN) { /* through the finally blocks it is in */
    wvdl_o(x, "return ");
    wvdl_o(x, x->fn->ctx);
    wvdl_o(x, ".a(\"r\", ");
    if (s->a != NULL) {
      wvdl_p_expr(x, s->a, PR_ASSIGN);
    } else {
      wvdl_o(x, "void 0");
    }
    wvdl_o(x, ");");
    return;
  }
  if (wvdl_in_iife(x)) {
    x->fn->codes |= 2;
    wvdl_o(x, "return {v: ");
    if (s->a != NULL) {
      wvdl_p_expr(x, s->a, PR_ASSIGN);
    } else {
      wvdl_o(x, "void 0");
    }
    wvdl_o(x, "};");
    return;
  }
  wvdl_o(x, "return");
  if (s->a != NULL) {
    wvdl_o(x, " ");
    wvdl_p_expr(x, s->a, PR_SEQ);
  }
  wvdl_o(x, ";");
}

/*
 * A loop body; `pre` goes first inside it (for-of / for-in heads).
 * When a closure made in the body keeps a let of it, the body runs as a
 * function each turn, so each turn has its own copy:
 *
 *   for (let i = 0; i < 3; i++) fns.push(() => i);
 *   ->
 *   for (var i = 0; i < 3; i++) { (function (i) {
 *     fns.push(function () { return i; }); }(i)); }
 *
 * break / continue / return inside come back as a code the loop acts on.
 */
static void wvdl_p_body(struct wvdl *x, wvdl_node *loop, const char *pre,
                        wvdl_node *params, const char *args) {
  wvdl_node *body = loop->d, blk;
  struct wvdl_fspec f;
  struct wvdl_buf fin, after;
  struct wvdl_cap cap;
  struct wvdl_code *c;
  const char *r = NULL, *fn_text;
  if (!(loop->extra & WVDL_X_IIFE)) {
    if (pre == NULL) {
      wvdl_p_stmt(x, body);
      return;
    }
    wvdl_o(x, " {");
    wvdl_o(x, pre);
    if (body->type == N_BLOCK) {
      wvdl_p_stmts(x, body->list);
    } else {
      wvdl_p_stmt(x, body);
    }
    wvdl_o(x, " }");
    return;
  }
  memset(&fin, 0, sizeof(fin));
  memset(&after, 0, sizeof(after));
  fin.arena = &x->arena;
  after.arena = &x->arena;
  if ((loop->type == N_FOR) && (loop->scope != NULL)) {
    struct wvdl_binding *b;
    for (b = loop->scope->first; b != NULL; b = b->next) {
      if (b->assigned_in_body) { /* for (let i...) { i += 2; ... } */
        const char *t = wvdl_temp(x);
        wvdl_puts(&fin, wvdl_fmt(x, " %s = %s;", t, b->out));
        wvdl_puts(&after, wvdl_fmt(x, " %s = %s;", b->out, t));
      }
    }
  }
  if (body->type != N_BLOCK) {
    memset(&blk, 0, sizeof(blk));
    blk.type = N_BLOCK;
    blk.pos = body->pos;
    blk.list = body;
    body = &blk;
  }
  memset(&f, 0, sizeof(f));
  f.kind = FN_LOOP;
  f.params = params;
  f.body = body;
  f.fin = (fin.len > 0) ? fin.p : NULL;
  wvdl_o(x, " {");
  if (pre != NULL) {
    wvdl_o(x, pre);
  }
  wvdl_cap_begin(x, &cap);
  wvdl_p_fn(x, &f);
  fn_text = wvdl_cap_end(x, &cap);
  wvdl_o(x, " ");
  if (f.codes || (f.labels != NULL)) {
    r = wvdl_temp(x);
    wvdl_o(x, wvdl_fmt(x, "%s = %s(%s);", r, fn_text, args ? args : ""));
  } else {
    wvdl_o(x, wvdl_fmt(x, "(%s(%s));", fn_text, args ? args : ""));
  }
  if (after.len > 0) {
    wvdl_o(x, after.p);
  }
  if (f.codes & 1) {
    wvdl_o(x, wvdl_fmt(x, " if (%s === \"b\") break;", r));
  }
  for (c = f.labels; c != NULL; c = c->next) {
    wvdl_node j;
    memset(&j, 0, sizeof(j));
    j.type = c->is_break ? N_BREAK : N_CONTINUE;
    j.str = c->label;
    j.len = c->len;
    wvdl_o(x, wvdl_fmt(x, " if (%s === \"%s\") ", r, c->text));
    wvdl_p_jump(x, &j);
  }
  if (f.codes & 2) {
    wvdl_node ret, raw;
    memset(&ret, 0, sizeof(ret));
    memset(&raw, 0, sizeof(raw));
    raw.type = N_RAW;
    raw.str = wvdl_fmt(x, "%s.v", r);
    ret.type = N_RETURN;
    ret.a = &raw;
    wvdl_o(x, wvdl_fmt(x, " if (%s) ", r));
    wvdl_p_return(x, &ret);
  }
  wvdl_o(x, " }");
}

/* the let / const names of a for (...) head, as parameters and arguments */
static wvdl_node *wvdl_head_params(struct wvdl *x, wvdl_node *loop,
                                   const char **args) {
  struct wvdl_binding *b;
  wvdl_node *head = NULL, *tail = NULL;
  struct wvdl_buf a;
  memset(&a, 0, sizeof(a));
  a.arena = &x->arena;
  if (loop->scope != NULL) {
    for (b = loop->scope->first; b != NULL; b = b->next) {
      wvdl_node *id = wvdl_new(x, N_ID, -1);
      id->str = b->name;
      id->len = b->len;
      id->binding = b;
      wvdl_append(&head, &tail, id);
      if (a.len > 0) {
        wvdl_put(&a, ", ", 2);
      }
      wvdl_puts(&a, b->out);
    }
  }
  *args = (a.p != NULL) ? a.p : "";
  return head;
}

/* for-in / for-of head: the statement putting `value` into it */
static const char *wvdl_head_assign(struct wvdl *x, wvdl_node *h,
                                    const char *value) {
  struct wvdl_cap c;
  int first = 1;
  wvdl_cap_begin(x, &c);
  wvdl_o(x, " ");
  if (h->type == N_VAR) {
    int hoist = wvdl_hoisting(x, h->op);
    if (!hoist) {
      wvdl_o(x, "var ");
    }
    wvdl_p_destructure(x, h->list->a, value, hoist ? D_HOIST : D_DECL, &first);
  } else {
    wvdl_p_destructure(x, h, value, D_ASSIGN, &first);
  }
  wvdl_o(x, ";");
  return wvdl_cap_end(x, &c);
}

/* `label` (or NULL) is printed on the loop itself: a for-of sits in a try */
static void wvdl_p_loop(struct wvdl *x, wvdl_node *n, wvdl_node *label) {
  struct wvdl_jump j;
  int iife = (n->extra & WVDL_X_IIFE) != 0;
  j.up = x->jumps;
  j.kind = J_LOOP;
  j.label = NULL;
  j.len = 0;
  switch (n->type) {
  case N_WHILE:
    wvdl_o(x, "while (");
    wvdl_p_expr(x, n->a, PR_SEQ);
    wvdl_o(x, ")");
    x->jumps = &j;
    wvdl_p_body(x, n, NULL, NULL, NULL);
    break;
  case N_DO:
    wvdl_o(x, "do");
    x->jumps = &j;
    wvdl_p_body(x, n, NULL, NULL, NULL);
    x->jumps = j.up;
    wvdl_o(x, " while (");
    wvdl_p_expr(x, n->a, PR_SEQ);
    wvdl_o(x, ");");
    break;
  case N_FOR: {
    const char *args = NULL;
    wvdl_node *params = NULL;
    int saved = x->no_in;
    wvdl_o(x, "for (");
    x->no_in = 1;
    if (n->a != NULL) {
      if (n->a->type == N_VAR) {
        wvdl_p_var(x, n->a, 1);
      } else {
        wvdl_p_expr(x, n->a, PR_SEQ);
      }
    }
    x->no_in = saved;
    wvdl_o(x, "; ");
    if (n->b != NULL) {
      wvdl_p_expr(x, n->b, PR_SEQ);
    }
    wvdl_o(x, "; ");
    if (n->c != NULL) {
      wvdl_p_expr(x, n->c, PR_SEQ);
    }
    wvdl_o(x, ")");
    if (iife) {
      params = wvdl_head_params(x, n, &args);
    }
    x->jumps = &j;
    wvdl_p_body(x, n, NULL, params, args);
    break;
  }
  case N_FOR_IN:
  case N_FOR_OF: {
    wvdl_node *h = n->a, *params = NULL;
    const char *pre = NULL, *args = NULL, *value;
    int block = (h->type == N_VAR) && (h->op != K_VAR);
    if (n->type == N_FOR_IN) {
      int saved = x->no_in;
      wvdl_o(x, "for (");
      if ((h->type == N_VAR) && (h->list->a->type == N_ID)) {
        const char *name = wvdl_id_out(x, h->list->a);
        if (wvdl_hoisting(x, h->op)) {
          wvdl_hoist(x, name);
        } else {
          wvdl_o(x, "var ");
        }
        wvdl_o(x, name);
        value = name;
      } else if ((h->type == N_ID) || (h->type == N_MEMBER)) {
        wvdl_p_expr(x, h, PR_CALL);
        value = NULL;
      } else {
        value = wvdl_temp(x);
        wvdl_o(x, value);
        pre = "";
      }
      x->no_in = 0;
      wvdl_o(x, " in ");
      wvdl_p_expr(x, n->b, PR_SEQ);
      x->no_in = saved;
      wvdl_o(x, ")");
      if (iife && block) {
        params = h->list->a;
        args = value;
        pre = NULL;
      } else if (pre != NULL) {
        pre = wvdl_head_assign(x, h, value);
      }
    } else {
      /* leaving early (break, return, throw) closes the iterator, which
         is what runs a generator's finally */
      const char *it = wvdl_temp(x), *st = wvdl_temp(x);
      wvdl_o(x, "try { ");
      if (label != NULL) {
        wvdl_on(x, label->str, label->len);
        wvdl_o(x, ": ");
      }
      wvdl_o(x, wvdl_fmt(x, "for (%s = __wv.it(", it));
      wvdl_p_expr(x, n->b, PR_ASSIGN);
      wvdl_o(x, wvdl_fmt(x, "), %s = null; !(%s = %s.next()).done; )", st, st, it));
      value = wvdl_fmt(x, "%s.value", st);
      if (iife && block) {
        params = h->list->a;
        args = value;
      } else {
        pre = wvdl_head_assign(x, h, value);
      }
      x->jumps = &j;
      wvdl_p_body(x, n, pre, params, args);
      wvdl_o(x, wvdl_fmt(x, " } finally { if (%s && !%s.done) __wv.close(%s); }",
                         st, st, it));
      break;
    }
    x->jumps = &j;
    wvdl_p_body(x, n, pre, params, args);
    break;
  }
  default:
    break;
  }
  x->jumps = j.up;
}

static void wvdl_p_stmt(struct wvdl *x, wvdl_node *s) {
  wvdl_stmt_start(x, s->pos);
  switch (s->type) {
  case N_EXPR_STMT: {
    const char *e = wvdl_expr_str(x, s->a, PR_SEQ);
    if ((e[0] == '{') ||
        ((strncmp(e, "function", 8) == 0) &&
         !wvdl_id_part((unsigned char)e[8]))) {
      wvdl_o(x, "(");
      wvdl_o(x, e);
      wvdl_o(x, ");");
    } else {
      wvdl_o(x, e);
      wvdl_o(x, ";");
    }
    return;
  }
  case N_VAR:
    wvdl_p_var(x, s, 0);
    wvdl_o(x, ";");
    return;
  case N_FUNC:
    wvdl_p_func_node(x, s, 0);
    return;
  case N_CLASS:
    if (wvdl_hoisting(x, K_LET)) {
      wvdl_hoist(x, wvdl_id_out(x, s->a));
    } else {
      wvdl_o(x, "var ");
    }
    wvdl_o(x, wvdl_id_out(x, s->a));
    wvdl_o(x, " = ");
    wvdl_p_class(x, s, NULL);
    wvdl_o(x, ";");
    return;
  case N_BLOCK:
    wvdl_o(x, "{");
    wvdl_p_stmts(x, s->list);
    wvdl_o(x, " }");
    return;
  case N_IF:
    wvdl_o(x, "if (");
    wvdl_p_expr(x, s->a, PR_SEQ);
    wvdl_o(x, ")");
    wvdl_p_stmt(x, s->b);
    if (s->c != NULL) {
      wvdl_o(x, " else");
      wvdl_p_stmt(x, s->c);
    }
    return;
  case N_FOR:
  case N_FOR_IN:
  case N_FOR_OF:
  case N_WHILE:
  case N_DO:
    wvdl_p_loop(x, s, NULL);
    return;
  case N_RETURN:
    wvdl_p_return(x, s);
    return;
  case N_BREAK:
  case N_CONTINUE:
    wvdl_p_jump(x, s);
    return;
  case N_THROW:
    wvdl_o(x, "throw ");
    wvdl_p_expr(x, s->a, PR_SEQ);
    wvdl_o(x, ";");
    return;
  case N_TRY:
    wvdl_o(x, "try");
    wvdl_p_stmt(x, s->a);
    if (s->c != NULL) {
      wvdl_o(x, " catch (");
      if (s->b->type == N_ID) {
        wvdl_p_id(x, s->b);
        wvdl_o(x, ")");
        wvdl_p_stmt(x, s->c);
      } else {
        const char *t = wvdl_fresh(x);
        int first = 1;
        wvdl_o(x, t);
        wvdl_o(x, ") { var ");
        wvdl_p_destructure(x, s->b, t, D_DECL, &first);
        wvdl_o(x, ";");
        wvdl_p_stmts(x, s->c->list);
        wvdl_o(x, " }");
      }
    }
    if (s->d != NULL) {
      wvdl_o(x, " finally");
      wvdl_p_stmt(x, s->d);
    }
    return;
  case N_SWITCH: {
    struct wvdl_jump j;
    wvdl_node *c;
    j.up = x->jumps;
    j.kind = J_SWITCH;
    j.label = NULL;
    j.len = 0;
    wvdl_o(x, "switch (");
    wvdl_p_expr(x, s->a, PR_SEQ);
    wvdl_o(x, ") {");
    x->jumps = &j;
    for (c = s->list; c != NULL; c = c->next) {
      wvdl_stmt_start(x, c->pos);
      if (c->a != NULL) {
        wvdl_o(x, "case ");
        wvdl_p_expr(x, c->a, PR_SEQ);
        wvdl_o(x, ":");
      } else {
        wvdl_o(x, "default:");
      }
      wvdl_p_stmts(x, c->list);
    }
    x->jumps = j.up;
    wvdl_o(x, " }");
    return;
  }
  case N_LABEL:
    if (s->op == 1) { /* with */
      wvdl_o(x, "with (");
      wvdl_p_expr(x, s->a, PR_SEQ);
      wvdl_o(x, ")");
      wvdl_p_stmt(x, s->b);
    } else {
      struct wvdl_jump j;
      j.up = x->jumps;
      j.kind = J_LABEL;
      j.label = s->str;
      j.len = s->len;
      x->jumps = &j;
      if (s->b->type == N_FOR_OF) { /* the label goes inside its try */
        wvdl_stmt_start(x, s->b->pos);
        wvdl_p_loop(x, s->b, s);
      } else {
        wvdl_on(x, s->str, s->len);
        wvdl_o(x, ":");
        wvdl_p_stmt(x, s->b);
      }
      x->jumps = j.up;
    }
    return;
  case N_EMPTY:
    wvdl_o(x, ";");
    return;
  case N_DEBUGGER:
    wvdl_o(x, "debugger;");
    return;
  default:
    break;
  }
  wvdl_error(x, s->pos, "unexpected statement");
}

/* statements of a block; renamed block functions go first, as variables */
static void wvdl_p_stmts(struct wvdl *x, wvdl_node *list) {
  wvdl_node *s;
  for (s = list; s != NULL; s = s->next) {
    if ((s->type == N_FUNC) && (s->flags & WVDL_F_DECL) &&
        (s->a->binding != NULL) && s->a->binding->renamed) {
      wvdl_stmt_start(x, s->pos);
      wvdl_o(x, "var ");
      wvdl_o(x, s->a->binding->out);
      wvdl_o(x, " = ");
      wvdl_p_func_node(x, s, 1);
      wvdl_o(x, ";");
      s->extra |= WVDL_X_DONE;
    }
  }
  for (s = list; s != NULL; s = s->next) {
    if (!(s->extra & WVDL_X_DONE)) {
      wvdl_p_stmt(x, s);
    }
  }
}

/* ====================================================== state machines */

/*
 * Generators and async functions. The body becomes a function the runtime
 * calls at every resume; a switch on the context's location picks the
 * place to go on from:
 *
 *   async function f(u) { var r = await get(u); return r.text; }
 *   ->
 *   function f(u) { var r; return __wv.async(function ($c) {
 *     while (1) switch ($c.p = $c.n) {
 *     case 0: $c.n = 1; return get(u);
 *     case 1: $c.p = 1; r = $c.sent; return $c.a("r", r.text);
 *     case "end": return $c.stop(); } }, []); }
 *
 * Only statements holding a yield / await (or a jump out of themselves) are
 * taken apart; the others are printed as usual inside the case they fall
 * in. Locations are written as placeholders and numbered at the end, in
 * the order they appear, so try regions are ranges the runtime can test.
 */

struct wvdl_gtarget { /* break / continue target in taken-apart code */
  struct wvdl_gtarget *up;
  const char *label;
  int len;
  int brk, cont; /* locations; cont < 0: not a loop */
  int is_switch;
  int finally_depth; /* finally blocks around it */
};

struct wvdl_gen {
  const char *c; /* context variable */
  int *loc;      /* location id -> number */
  int locs, cap;
  int marks;
  struct wvdl_buf tries; /* "[try, catch, finally, after], ..." */
  struct wvdl_gtarget *targets;
  int finally_depth;
  int async, agen;
};

static int wvdl_mark_yield(wvdl_node *n) {
  int y = 0;
  wvdl_node *q;
  if ((n == NULL) || (n->type == N_FUNC) || (n->type == N_CLASS)) {
    return 0; /* functions inside are on their own */
  }
  if ((n->type == N_YIELD) || (n->type == N_AWAIT) ||
      ((n->type == N_FOR_OF) && n->flags)) {
    y = 1;
  }
  y |= wvdl_mark_yield(n->a);
  y |= wvdl_mark_yield(n->b);
  y |= wvdl_mark_yield(n->c);
  y |= wvdl_mark_yield(n->d);
  for (q = n->list; q != NULL; q = q->next) {
    y |= wvdl_mark_yield(q);
  }
  if (y) {
    n->extra |= WVDL_X_YIELD;
  }
  return y;
}

struct wvdl_glabel {
  struct wvdl_glabel *up;
  const char *s;
  int n;
};

/* does a break / continue inside s leave s? */
static int wvdl_jumps_out(wvdl_node *s, int loops, int switches,
                          struct wvdl_glabel *labels) {
  struct wvdl_glabel *l, here;
  wvdl_node *q;
  if (s == NULL) {
    return 0;
  }
  switch (s->type) {
  case N_BREAK:
  case N_CONTINUE:
    if (s->str != NULL) {
      for (l = labels; l != NULL; l = l->up) {
        if ((l->n == s->len) && (memcmp(l->s, s->str, (size_t)s->len) == 0)) {
          return 0;
        }
      }
      return 1;
    }
    return (s->type == N_BREAK) ? (loops + switches == 0) : (loops == 0);
  case N_BLOCK:
    for (q = s->list; q != NULL; q = q->next) {
      if (wvdl_jumps_out(q, loops, switches, labels)) {
        return 1;
      }
    }
    return 0;
  case N_IF:
    return wvdl_jumps_out(s->b, loops, switches, labels) ||
           wvdl_jumps_out(s->c, loops, switches, labels);
  case N_FOR:
  case N_FOR_IN:
  case N_FOR_OF:
  case N_WHILE:
  case N_DO:
    return wvdl_jumps_out(s->d, loops + 1, switches, labels);
  case N_LABEL:
    if (s->op == 1) {
      return wvdl_jumps_out(s->b, loops, switches, labels);
    }
    here.up = labels;
    here.s = s->str;
    here.n = s->len;
    return wvdl_jumps_out(s->b, loops, switches, &here);
  case N_TRY:
    return wvdl_jumps_out(s->a, loops, switches, labels) ||
           wvdl_jumps_out(s->c, loops, switches, labels) ||
           wvdl_jumps_out(s->d, loops, switches, labels);
  case N_SWITCH:
    for (q = s->list; q != NULL; q = q->next) {
      wvdl_node *t;
      for (t = q->list; t != NULL; t = t->next) {
        if (wvdl_jumps_out(t, loops, switches + 1, labels)) {
          return 1;
        }
      }
    }
    return 0;
  default:
    return 0;
  }
}

static int wvdl_g_needs(wvdl_node *s) {
  return (s->extra & WVDL_X_YIELD) || wvdl_jumps_out(s, 0, 0, NULL);
}

static int wvdl_g_loc(struct wvdl *x, struct wvdl_gen *g) {
  if (g->locs == g->cap) {
    int cap = g->cap ? g->cap * 2 : 64;
    int *p = (int *)wvdl_alloc(&x->arena, (size_t)cap * sizeof(int));
    if (p == NULL) {
      wvdl_error(x, 0, "out of memory");
    }
    if (g->locs > 0) {
      memcpy(p, g->loc, (size_t)g->locs * sizeof(int));
    }
    g->loc = p;
    g->cap = cap;
  }
  g->loc[g->locs] = -1;
  return g->locs++;
}

/* a location in the text, numbered once all is printed */
static void wvdl_g_ph(struct wvdl *x, int id) {
  wvdl_o(x, wvdl_fmt(x, "\001%d\002", id));
}

static void wvdl_g_mark(struct wvdl *x, struct wvdl_gen *g, int id) {
  g->loc[id] = ++g->marks;
  wvdl_o(x, " case ");
  wvdl_g_ph(x, id);
  wvdl_o(x, wvdl_fmt(x, ": %s.p = ", g->c));
  wvdl_g_ph(x, id);
  wvdl_o(x, ";");
}

static void wvdl_g_jump(struct wvdl *x, struct wvdl_gen *g, int id) {
  wvdl_o(x, wvdl_fmt(x, " %s.n = ", g->c));
  wvdl_g_ph(x, id);
  wvdl_o(x, "; break;");
}

/* if (!(test)) jump: `negate` 0 jumps when test holds */
static void wvdl_g_branch(struct wvdl *x, struct wvdl_gen *g, const char *test,
                          int negate, int id) {
  wvdl_o(x, negate ? " if (!(" : " if (");
  wvdl_o(x, test);
  wvdl_o(x, negate ? ")) {" : ") {");
  wvdl_g_jump(x, g, id);
  wvdl_o(x, " }");
}

static wvdl_node *wvdl_raw(struct wvdl *x, const char *text, int pos) {
  wvdl_node *n = wvdl_new(x, N_RAW, pos);
  n->str = text;
  return n;
}

static wvdl_node *wvdl_clone(struct wvdl *x, wvdl_node *n) {
  wvdl_node *c = wvdl_new(x, n->type, n->pos);
  *c = *n;
  return c;
}

/* copies of the items of a list; returns the new head */
static wvdl_node *wvdl_clone_list(struct wvdl *x, wvdl_node *l) {
  wvdl_node *head = NULL, *tail = NULL;
  for (; l != NULL; l = l->next) {
    wvdl_node *c = wvdl_clone(x, l);
    c->next = NULL;
    wvdl_append(&head, &tail, c);
  }
  return head;
}

static wvdl_node *wvdl_g_expr(struct wvdl *x, struct wvdl_gen *g, wvdl_node *n);

static const char *wvdl_g_text(struct wvdl *x, struct wvdl_gen *g,
                               wvdl_node *n, int prec) {
  return wvdl_expr_str(x, wvdl_g_expr(x, g, n), prec);
}

/* "t = value;" */
static const char *wvdl_g_save(struct wvdl *x, const char *value) {
  const char *t = wvdl_temp(x);
  wvdl_o(x, wvdl_fmt(x, " %s = %s;", t, value));
  return t;
}

/* yield / await: stop here, go on at a new location with what is sent */
static const char *wvdl_g_suspend(struct wvdl *x, struct wvdl_gen *g,
                                  wvdl_node *n) {
  int k = wvdl_g_loc(x, g);
  int star = (n->type == N_YIELD) && n->flags;
  const char *arg = (n->a != NULL) ? wvdl_g_text(x, g, n->a, PR_ASSIGN)
                                   : "void 0";
  wvdl_o(x, wvdl_fmt(x, " %s.n = ", g->c));
  wvdl_g_ph(x, k);
  if (star) {
    if (g->agen) {
      wvdl_unsupported(x, n, "yield* in async generators");
    }
    wvdl_o(x, wvdl_fmt(x, "; return %s.d(%s);", g->c, arg));
  } else if ((n->type == N_AWAIT) && g->agen) {
    wvdl_o(x, wvdl_fmt(x, "; return new __wv.Aw(%s);", arg));
  } else {
    wvdl_o(x, wvdl_fmt(x, "; return %s;", arg));
  }
  wvdl_g_mark(x, g, k);
  return wvdl_fmt(x, star ? "%s.dv" : "%s.sent", g->c);
}

static int wvdl_g_pure(wvdl_node *n) {
  switch (n->type) {
  case N_NUM: case N_STR: case N_LIT: case N_REGEX: case N_THIS: case N_FUNC:
  case N_RAW: case N_HOLE:
    return 1;
  default:
    return 0;
  }
}

/* children evaluated before a yield are kept in temporaries, so they are
   evaluated in order; the node is rebuilt around what comes back */
static wvdl_node *wvdl_g_children(struct wvdl *x, struct wvdl_gen *g,
                                  wvdl_node *n) {
  wvdl_node *slot[128], *c = wvdl_clone(x, n), *q;
  int ns = 0, i, last = -1;
#define WVDL_SLOT(p)                                                         \
  do {                                                                       \
    if (ns == 128) {                                                         \
      wvdl_unsupported(x, n, "this many values around a yield");             \
    }                                                                        \
    slot[ns++] = (p);                                                        \
  } while (0)
  switch (c->type) {
  case N_ASSIGN:
  case N_UPDATE:
    if (c->a->type == N_MEMBER) {
      c->a = wvdl_clone(x, c->a);
      if (c->a->a->type != N_SUPER) {
        WVDL_SLOT(c->a->a);
      }
      if (c->a->flags & WVDL_F_COMPUTED) {
        WVDL_SLOT(c->a->b);
      }
    } else if ((c->a->type != N_ID) && (c->a->extra & WVDL_X_YIELD)) {
      wvdl_unsupported(x, n, "a yield inside a destructuring target");
    }
    if (c->type == N_ASSIGN) {
      WVDL_SLOT(c->b);
    }
    break;
  case N_CALL:
  case N_NEW:
  case N_TAGGED:
    if ((c->a->type == N_MEMBER) && (c->a->a->type != N_SUPER)) {
      c->a = wvdl_clone(x, c->a); /* keep obj.f(...) a method call */
      WVDL_SLOT(c->a->a);
      if (c->a->flags & WVDL_F_COMPUTED) {
        WVDL_SLOT(c->a->b);
      }
    } else if ((c->a->type != N_ID) && (c->a->type != N_SUPER)) {
      WVDL_SLOT(c->a);
    }
    if (c->type == N_TAGGED) {
      c->b = wvdl_clone(x, c->b);
      c->b->list = wvdl_clone_list(x, c->b->list);
      for (q = c->b->list; q != NULL; q = q->next) {
        if (!((q->type == N_STR) && (q->a != NULL))) {
          wvdl_unsupported(x, n, "a yield inside a tagged template");
        }
      }
      break;
    }
    /* fall through */
  case N_ARRAY:
  case N_SEQ:
  case N_TEMPLATE:
    c->list = wvdl_clone_list(x, c->list);
    for (q = c->list; q != NULL; q = q->next) {
      if (q->type == N_SPREAD) {
        WVDL_SLOT(q->a);
      } else if ((q->type != N_HOLE) && !((q->type == N_STR) && (q->a != NULL))) {
        WVDL_SLOT(q);
      }
    }
    break;
  case N_OBJECT:
    c->list = wvdl_clone_list(x, c->list);
    for (q = c->list; q != NULL; q = q->next) {
      if (q->flags & WVDL_F_COMPUTED) {
        WVDL_SLOT(q->a);
      }
      if ((q->op == P_INIT) || (q->op == P_SHORT) || (q->op == P_SPREAD)) {
        WVDL_SLOT(q->b);
      }
    }
    break;
  case N_MEMBER:
    if (c->a->type != N_SUPER) {
      WVDL_SLOT(c->a);
    }
    if (c->flags & WVDL_F_COMPUTED) {
      WVDL_SLOT(c->b);
    }
    break;
  case N_UNARY:
  case N_SPREAD:
    WVDL_SLOT(c->a);
    break;
  case N_BINARY:
    WVDL_SLOT(c->a);
    WVDL_SLOT(c->b);
    break;
  default:
    wvdl_unsupported(x, n, "a yield / await in this kind of expression");
  }
#undef WVDL_SLOT
  for (i = 0; i < ns; i++) {
    if (slot[i]->extra & WVDL_X_YIELD) {
      last = i;
    }
  }
  /* each slot is overwritten in place (lists keep their links) */
  for (i = 0; i < ns; i++) {
    wvdl_node *v = slot[i], *keep_next = v->next;
    wvdl_node *r;
    if (v->extra & WVDL_X_YIELD) {
      r = wvdl_g_expr(x, g, v);
      if (i < last) { /* another yield follows: evaluate it now */
        r = wvdl_raw(x, wvdl_g_save(x, wvdl_expr_str(x, r, PR_ASSIGN)), v->pos);
      }
    } else if ((i < last) && !wvdl_g_pure(v)) {
      r = wvdl_raw(x, wvdl_g_save(x, wvdl_expr_str(x, v, PR_ASSIGN)), v->pos);
    } else {
      continue;
    }
    if (r != v) {
      *v = *r;
      v->next = keep_next;
    }
  }
  c->extra &= ~WVDL_X_YIELD;
  return c;
}

static wvdl_node *wvdl_g_expr(struct wvdl *x, struct wvdl_gen *g,
                              wvdl_node *n) {
  if (!(n->extra & WVDL_X_YIELD)) {
    return n;
  }
  switch (n->type) {
  case N_YIELD:
  case N_AWAIT:
    return wvdl_raw(x, wvdl_g_save(x, wvdl_g_suspend(x, g, n)), n->pos);
  case N_BINARY: {
    const char *op = (n->op >= 0) ? wvdl_op_text(n->op) : "";
    if (!strcmp(op, "&&") || !strcmp(op, "||") || !strcmp(op, "??")) {
      /* r = a; if (r does not decide) r = b; */
      int skip = wvdl_g_loc(x, g);
      const char *r = wvdl_g_save(x, wvdl_g_text(x, g, n->a, PR_ASSIGN));
      wvdl_g_branch(x, g,
                    (op[0] == '?') ? wvdl_fmt(x, "%s != null", r) : r,
                    op[0] == '&', skip);
      wvdl_o(x, wvdl_fmt(x, " %s = %s;", r, wvdl_g_text(x, g, n->b, PR_ASSIGN)));
      wvdl_g_mark(x, g, skip);
      return wvdl_raw(x, r, n->pos);
    }
    return wvdl_g_children(x, g, n);
  }
  case N_COND: {
    int other = wvdl_g_loc(x, g), after = wvdl_g_loc(x, g);
    const char *r = wvdl_temp(x);
    wvdl_g_branch(x, g, wvdl_g_text(x, g, n->a, PR_SEQ), 1, other);
    wvdl_o(x, wvdl_fmt(x, " %s = %s;", r, wvdl_g_text(x, g, n->b, PR_ASSIGN)));
    wvdl_g_jump(x, g, after);
    wvdl_g_mark(x, g, other);
    wvdl_o(x, wvdl_fmt(x, " %s = %s;", r, wvdl_g_text(x, g, n->c, PR_ASSIGN)));
    wvdl_g_mark(x, g, after);
    return wvdl_raw(x, r, n->pos);
  }
  case N_ASSIGN: {
    const char *op = wvdl_op_text(n->op);
    if (!strcmp(op, "||=") || !strcmp(op, "&&=") || !strcmp(op, "?\?=")) {
      wvdl_unsupported(x, n, "a yield / await in a logical assignment");
    }
    return wvdl_g_children(x, g, n);
  }
  case N_CHAIN:
    wvdl_unsupported(x, n, "a yield / await inside an optional chain");
    return n;
  default:
    return wvdl_g_children(x, g, n);
  }
}

static void wvdl_g_stmt(struct wvdl *x, struct wvdl_gen *g, wvdl_node *s);

static void wvdl_g_stmts(struct wvdl *x, struct wvdl_gen *g, wvdl_node *l) {
  for (; l != NULL; l = l->next) {
    wvdl_g_stmt(x, g, l);
  }
}

static void wvdl_g_push(struct wvdl_gen *g, struct wvdl_gtarget *t,
                        const char *label, int len, int brk, int cont,
                        int is_switch) {
  t->up = g->targets;
  t->label = label;
  t->len = len;
  t->brk = brk;
  t->cont = cont;
  t->is_switch = is_switch;
  t->finally_depth = g->finally_depth;
  g->targets = t;
}

/* the head of a for-in / for-of gets `value` */
static void wvdl_g_head(struct wvdl *x, wvdl_node *h, const char *value) {
  int first = 1;
  wvdl_o(x, " ");
  if (h->type == N_VAR) {
    wvdl_p_destructure(x, h->list->a, value, D_HOIST, &first);
  } else {
    wvdl_p_destructure(x, h, value, D_ASSIGN, &first);
  }
  wvdl_o(x, ";");
}

static void wvdl_g_loop(struct wvdl *x, struct wvdl_gen *g, wvdl_node *s,
                        wvdl_node *label) {
  struct wvdl_gtarget t;
  const char *lab = label ? label->str : NULL;
  int len = label ? label->len : 0;
  int after = wvdl_g_loc(x, g), top = wvdl_g_loc(x, g), next;
  switch (s->type) {
  case N_WHILE:
    wvdl_g_mark(x, g, top);
    wvdl_g_branch(x, g, wvdl_g_text(x, g, s->a, PR_SEQ), 1, after);
    wvdl_g_push(g, &t, lab, len, after, top, 0);
    wvdl_g_stmt(x, g, s->d);
    break;
  case N_DO:
    next = wvdl_g_loc(x, g);
    wvdl_g_mark(x, g, top);
    wvdl_g_push(g, &t, lab, len, after, next, 0);
    wvdl_g_stmt(x, g, s->d);
    g->targets = t.up;
    wvdl_g_mark(x, g, next);
    wvdl_g_branch(x, g, wvdl_g_text(x, g, s->a, PR_SEQ), 0, top);
    wvdl_g_mark(x, g, after);
    return;
  case N_FOR:
    next = wvdl_g_loc(x, g);
    if (s->a != NULL) {
      wvdl_stmt_start(x, s->a->pos);
      if (s->a->type == N_VAR) {
        wvdl_node v = *s->a; /* run it through the statement path */
        v.type = N_VAR;
        wvdl_g_stmt(x, g, &v);
      } else {
        wvdl_o(x, wvdl_g_text(x, g, s->a, PR_SEQ));
        wvdl_o(x, ";");
      }
    }
    wvdl_g_mark(x, g, top);
    if (s->b != NULL) {
      wvdl_g_branch(x, g, wvdl_g_text(x, g, s->b, PR_SEQ), 1, after);
    }
    wvdl_g_push(g, &t, lab, len, after, next, 0);
    wvdl_g_stmt(x, g, s->d);
    g->targets = t.up;
    wvdl_g_mark(x, g, next);
    if (s->c != NULL) {
      wvdl_o(x, " ");
      wvdl_o(x, wvdl_g_text(x, g, s->c, PR_SEQ));
      wvdl_o(x, ";");
    }
    wvdl_g_jump(x, g, top);
    wvdl_g_mark(x, g, after);
    return;
  case N_FOR_IN:
  case N_FOR_OF: {
    const char *it = wvdl_temp(x), *st = wvdl_temp(x);
    const char *from = wvdl_g_text(x, g, s->b, PR_ASSIGN);
    wvdl_o(x, wvdl_fmt(x, " %s = __wv.%s(%s);", it,
                       (s->type == N_FOR_IN) ? "keys"
                       : s->flags            ? "ait"
                                             : "it",
                       from));
    wvdl_g_mark(x, g, top);
    if (s->flags) { /* for await: wait for each step */
      wvdl_node aw;
      memset(&aw, 0, sizeof(aw));
      aw.type = N_AWAIT;
      aw.pos = s->pos;
      aw.a = wvdl_raw(x, wvdl_fmt(x, "%s.next()", it), s->pos);
      wvdl_o(x, wvdl_fmt(x, " %s = %s;", st, wvdl_g_suspend(x, g, &aw)));
      wvdl_g_branch(x, g, wvdl_fmt(x, "%s.done", st), 0, after);
    } else {
      wvdl_g_branch(x, g, wvdl_fmt(x, "(%s = %s.next()).done", st, it), 0,
                    after);
    }
    wvdl_g_head(x, s->a, wvdl_fmt(x, "%s.value", st));
    wvdl_g_push(g, &t, lab, len, after, top, 0);
    wvdl_g_stmt(x, g, s->d);
    break;
  }
  default:
    return;
  }
  g->targets = t.up;
  wvdl_g_jump(x, g, top);
  wvdl_g_mark(x, g, after);
}

static void wvdl_g_leap(struct wvdl *x, struct wvdl_gen *g, wvdl_node *s) {
  struct wvdl_gtarget *t;
  int brk = (s->type == N_BREAK), loc;
  for (t = g->targets; t != NULL; t = t->up) {
    if (s->str != NULL) {
      if ((t->label != NULL) && (t->len == s->len) &&
          (memcmp(t->label, s->str, (size_t)s->len) == 0)) {
        break;
      }
    } else if (brk ? ((t->cont >= 0) || t->is_switch) : (t->cont >= 0)) {
      break;
    }
  }
  if ((t == NULL) || (!brk && (t->cont < 0))) {
    wvdl_error(x, s->pos, "jump target not found");
  }
  loc = brk ? t->brk : t->cont;
  if (t->finally_depth < g->finally_depth) { /* through finally blocks */
    wvdl_o(x, wvdl_fmt(x, " return %s.a(\"b\", ", g->c));
    wvdl_g_ph(x, loc);
    wvdl_o(x, ");");
  } else {
    wvdl_g_jump(x, g, loc);
  }
}

static void wvdl_g_try(struct wvdl *x, struct wvdl_gen *g, wvdl_node *s) {
  int t = wvdl_g_loc(x, g), c = (s->c != NULL) ? wvdl_g_loc(x, g) : -1;
  int f = (s->d != NULL) ? wvdl_g_loc(x, g) : -1, a = wvdl_g_loc(x, g);
  if (g->tries.len > 0) {
    wvdl_puts(&g->tries, ", ");
  }
  wvdl_puts(&g->tries, wvdl_fmt(x, "[\001%d\002, ", t));
  wvdl_puts(&g->tries, (c >= 0) ? wvdl_fmt(x, "\001%d\002, ", c) : "-1, ");
  wvdl_puts(&g->tries, (f >= 0) ? wvdl_fmt(x, "\001%d\002, ", f) : "-1, ");
  wvdl_puts(&g->tries, wvdl_fmt(x, "\001%d\002]", a));
  wvdl_g_mark(x, g, t);
  if (f >= 0) {
    g->finally_depth++;
  }
  wvdl_g_stmt(x, g, s->a);
  wvdl_g_jump(x, g, (f >= 0) ? f : a);
  if (c >= 0) {
    wvdl_g_mark(x, g, c);
    if (s->b->type == N_ID) {
      wvdl_hoist(x, wvdl_id_out(x, s->b));
      wvdl_o(x, wvdl_fmt(x, " %s = %s.ex(", wvdl_id_out(x, s->b), g->c));
      wvdl_g_ph(x, t);
      wvdl_o(x, ");");
    } else {
      struct wvdl_cap cap;
      int first = 1;
      wvdl_cap_begin(x, &cap);
      wvdl_o(x, g->c);
      wvdl_o(x, ".ex(");
      wvdl_g_ph(x, t);
      wvdl_o(x, ")");
      wvdl_o(x, " ");
      wvdl_p_destructure(x, s->b, wvdl_g_save(x, wvdl_cap_end(x, &cap)),
                         D_HOIST, &first);
      wvdl_o(x, ";");
    }
    wvdl_g_stmt(x, g, s->c);
    wvdl_g_jump(x, g, (f >= 0) ? f : a);
  }
  if (f >= 0) {
    g->finally_depth--;
    wvdl_g_mark(x, g, f);
    wvdl_g_stmt(x, g, s->d);
    wvdl_o(x, wvdl_fmt(x, " return %s.fin(", g->c));
    wvdl_g_ph(x, f);
    wvdl_o(x, ");");
  }
  wvdl_g_mark(x, g, a);
}

static void wvdl_g_switch(struct wvdl *x, struct wvdl_gen *g, wvdl_node *s) {
  struct wvdl_gtarget t;
  wvdl_node *c;
  int after = wvdl_g_loc(x, g), deflt = -1, count = 0, i;
  int *locs;
  const char *d = wvdl_g_save(x, wvdl_g_text(x, g, s->a, PR_ASSIGN));
  for (c = s->list; c != NULL; c = c->next) {
    count++;
  }
  locs = (int *)wvdl_alloc(&x->arena, (size_t)(count + 1) * sizeof(int));
  if (locs == NULL) {
    wvdl_error(x, s->pos, "out of memory");
  }
  for (c = s->list, i = 0; c != NULL; c = c->next, i++) {
    locs[i] = wvdl_g_loc(x, g);
    if (c->a == NULL) {
      deflt = locs[i];
    } else {
      wvdl_g_branch(x, g,
                    wvdl_fmt(x, "%s === %s", d,
                             wvdl_g_text(x, g, c->a, PR_EQ + 1)),
                    0, locs[i]);
    }
  }
  wvdl_g_jump(x, g, (deflt >= 0) ? deflt : after);
  wvdl_g_push(g, &t, NULL, 0, after, -1, 1);
  for (c = s->list, i = 0; c != NULL; c = c->next, i++) {
    wvdl_g_mark(x, g, locs[i]);
    wvdl_g_stmts(x, g, c->list);
  }
  g->targets = t.up;
  wvdl_g_mark(x, g, after);
}

static int wvdl_suspends(wvdl_node *n) {
  return (n != NULL) && ((n->type == N_YIELD) || (n->type == N_AWAIT));
}

static void wvdl_g_stmt(struct wvdl *x, struct wvdl_gen *g, wvdl_node *s) {
  wvdl_node *d;
  if (!wvdl_g_needs(s)) {
    wvdl_p_stmt(x, s);
    return;
  }
  wvdl_line_to(x, s->pos);
  switch (s->type) {
  case N_BLOCK:
    wvdl_g_stmts(x, g, s->list);
    return;
  case N_EXPR_STMT: {
    const char *e;
    if (wvdl_suspends(s->a)) { /* yield x; - the value is not wanted */
      if (s->a->a != NULL) {
        s->a->a = wvdl_g_expr(x, g, s->a->a);
      }
      wvdl_g_suspend(x, g, s->a);
      return;
    }
    e = wvdl_g_text(x, g, s->a, PR_SEQ);
    wvdl_o(x, " ");
    if ((e[0] == '{') || ((strncmp(e, "function", 8) == 0) &&
                          !wvdl_id_part((unsigned char)e[8]))) {
      wvdl_o(x, wvdl_fmt(x, "(%s);", e));
    } else {
      wvdl_o(x, wvdl_fmt(x, "%s;", e));
    }
    return;
  }
  case N_VAR:
    for (d = s->list; d != NULL; d = d->next) {
      const char *v;
      int first = 1;
      if (d->b == NULL) {
        if (d->a->type == N_ID) {
          wvdl_hoist(x, wvdl_id_out(x, d->a));
          if ((s->op != K_VAR) && (d->a->binding != NULL) &&
              !d->a->binding->scope->is_fn) {
            wvdl_o(x, wvdl_fmt(x, " %s = void 0;", wvdl_id_out(x, d->a)));
          }
        }
        continue;
      }
      if (wvdl_suspends(d->b)) {
        if (d->b->a != NULL) {
          d->b->a = wvdl_g_expr(x, g, d->b->a);
        }
        v = wvdl_g_suspend(x, g, d->b);
      } else {
        v = wvdl_g_text(x, g, d->b, PR_ASSIGN);
      }
      wvdl_o(x, " ");
      wvdl_p_destructure(x, d->a, v, D_HOIST, &first);
      wvdl_o(x, ";");
    }
    return;
  case N_IF: {
    int other = wvdl_g_loc(x, g), after = wvdl_g_loc(x, g);
    wvdl_g_branch(x, g, wvdl_g_text(x, g, s->a, PR_SEQ), 1,
                  (s->c != NULL) ? other : after);
    wvdl_g_stmt(x, g, s->b);
    if (s->c != NULL) {
      wvdl_g_jump(x, g, after);
      wvdl_g_mark(x, g, other);
      wvdl_g_stmt(x, g, s->c);
    }
    wvdl_g_mark(x, g, after);
    return;
  }
  case N_WHILE:
  case N_DO:
  case N_FOR:
  case N_FOR_IN:
  case N_FOR_OF:
    wvdl_g_loop(x, g, s, NULL);
    return;
  case N_LABEL:
    if (s->op == 1) {
      wvdl_unsupported(x, s, "a yield / await inside with");
    }
    if ((s->b->type == N_WHILE) || (s->b->type == N_DO) ||
        (s->b->type == N_FOR) || (s->b->type == N_FOR_IN) ||
        (s->b->type == N_FOR_OF)) {
      wvdl_g_loop(x, g, s->b, s);
    } else {
      struct wvdl_gtarget t;
      int after = wvdl_g_loc(x, g);
      wvdl_g_push(g, &t, s->str, s->len, after, -1, 0);
      wvdl_g_stmt(x, g, s->b);
      g->targets = t.up;
      wvdl_g_mark(x, g, after);
    }
    return;
  case N_BREAK:
  case N_CONTINUE:
    wvdl_g_leap(x, g, s);
    return;
  case N_RETURN:
    wvdl_o(x, wvdl_fmt(x, " return %s.a(\"r\", %s);", g->c,
                       (s->a != NULL) ? wvdl_g_text(x, g, s->a, PR_ASSIGN)
                                      : "void 0"));
    return;
  case N_THROW:
    wvdl_o(x, wvdl_fmt(x, " throw %s;", wvdl_g_text(x, g, s->a, PR_SEQ)));
    return;
  case N_TRY:
    wvdl_g_try(x, g, s);
    return;
  case N_SWITCH:
    wvdl_g_switch(x, g, s);
    return;
  default:
    wvdl_unsupported(x, s, "a yield / await in this statement");
  }
}

/* numbers for the placeholders, in the order they were marked */
static const char *wvdl_g_number(struct wvdl *x, struct wvdl_gen *g,
                                 const char *text) {
  struct wvdl_buf b;
  memset(&b, 0, sizeof(b));
  b.arena = &x->arena;
  while (*text) {
    const char *p = strchr(text, '\001');
    int id;
    if (p == NULL) {
      wvdl_puts(&b, text);
      break;
    }
    wvdl_put(&b, text, (size_t)(p - text));
    id = atoi(p + 1);
    if ((id < 0) || (id >= g->locs) || (g->loc[id] < 0)) {
      wvdl_error(x, 0, "internal error: location %d not marked", id);
    }
    wvdl_puts(&b, wvdl_fmt(x, "%d", g->loc[id]));
    text = strchr(p, '\002') + 1;
  }
  if (b.failed) {
    wvdl_error(x, 0, "out of memory");
  }
  return b.p ? b.p : "";
}

/* the body of a generator / async function: return __wv.gen(...) */
static void wvdl_p_genbody(struct wvdl *x, struct wvdl_fspec *f) {
  struct wvdl_gen g;
  struct wvdl_fn gfn;
  struct wvdl_jump jb;
  struct wvdl_cap cap;
  const char *text, *driver;
  memset(&g, 0, sizeof(g));
  g.c = wvdl_fresh(x);
  g.async = (f->gen & WVDL_F_ASYNC) != 0;
  g.agen = g.async && (f->gen & WVDL_F_GEN);
  g.tries.arena = &x->arena;
  memset(&gfn, 0, sizeof(gfn));
  gfn.kind = FN_GEN;
  gfn.up = x->fn;
  gfn.ctx = g.c;
  gfn.vars.arena = &x->arena;
  jb.up = x->jumps;
  jb.kind = J_FUNC;
  jb.label = NULL;
  jb.len = 0;
  x->fn = &gfn;
  x->jumps = &jb;
  wvdl_cap_begin(x, &cap);
  wvdl_o(x, " case 0:");
  if (f->expr_body) { /* async x => await y */
    wvdl_node ret;
    memset(&ret, 0, sizeof(ret));
    ret.type = N_RETURN;
    ret.pos = f->body->pos;
    ret.a = f->body;
    wvdl_mark_yield(&ret);
    wvdl_g_stmt(x, &g, &ret);
  } else {
    wvdl_node *q;
    for (q = f->body->list; q != NULL; q = q->next) {
      wvdl_mark_yield(q);
    }
    wvdl_g_stmts(x, &g, f->body->list);
  }
  wvdl_o(x, wvdl_fmt(x, " case \"end\": return %s.stop();", g.c));
  text = wvdl_g_number(x, &g, wvdl_cap_end(x, &cap));
  x->fn = gfn.up;
  x->jumps = jb.up;
  driver = g.agen ? "agen" : g.async ? "async" : "gen";
  wvdl_o(x, wvdl_fmt(x, " return __wv.%s(function (%s) { while (1) switch (%s.p = %s.n) {",
                     driver, g.c, g.c, g.c));
  wvdl_o(x, text);
  wvdl_o(x, " } }, [");
  if (g.tries.len > 0) {
    wvdl_o(x, wvdl_g_number(x, &g, g.tries.p));
  }
  wvdl_o(x, "]);");
}

/* ================================================================== API */

static int wvdl_contains(const char *s, int n, const char *what) {
  int m = (int)strlen(what), i;
  for (i = 0; i + m <= n; i++) {
    if ((s[i] == what[0]) && (memcmp(s + i, what, (size_t)m) == 0)) {
      return 1;
    }
  }
  return 0;
}

/* tells this script's template literals apart from other scripts' */
static void wvdl_tpl_id(struct wvdl *x) {
  unsigned h = 2166136261u;
  int i;
  for (i = 0; i < x->len; i++) {
    h = (h ^ (unsigned char)x->src[i]) * 16777619u;
  }
  snprintf(x->tpl_id, sizeof(x->tpl_id), "%08x", h);
}

static char *wvdl_copy(const char *s, size_t n) {
  char *r = (char *)malloc(n + 1);
  if (r != NULL) {
    memcpy(r, s, n);
    r[n] = 0;
  }
  return r;
}

/*
 * Modern JavaScript in, ES3 out (malloc'd; free it). A script without
 * anything newer than ES3 comes back as it was. NULL on a syntax error or
 * a feature that cannot be translated yet, with the reason in err.
 */
/* WEBVIEW_DOWNLEVEL_OLD_IE for IE8 and older (see "IE8 and older: names") */
WEBVIEW_API void webview_downlevel_mode(int flags) {
  wvdl_mode_flags = flags;
}

WEBVIEW_API char *webview_downlevel(const char *src, size_t len, char *err,
                                    size_t errlen) {
  struct wvdl *x;
  struct wvdl_buf out;
  struct wvdl_fn top;
  struct wvdl_scope *root;
  struct wvdl_cap cap;
  wvdl_node *prog;
  const char *body;
  char *result;
  if ((err != NULL) && (errlen > 0)) {
    err[0] = 0;
  }
  if (len > 0x3FFFFFFF) {
    if ((err != NULL) && (errlen > 0)) {
      snprintf(err, errlen, "script too large");
    }
    return NULL;
  }
  /* conditional compilation lives in comments, which are not kept */
  if (wvdl_contains(src, (int)len, "@cc_on")) {
    return wvdl_copy(src, len);
  }
  x = (struct wvdl *)calloc(1, sizeof(struct wvdl));
  if (x == NULL) {
    return NULL;
  }
  x->src = src;
  x->len = (int)len;
  x->err = err;
  x->errlen = errlen;
  if (setjmp(x->fail)) {
    wvdl_arena_free(&x->arena);
    free(x);
    return NULL;
  }
  prog = wvdl_program(x);
  if (!x->modern) {
    wvdl_arena_free(&x->arena);
    free(x);
    return wvdl_copy(src, len);
  }
  x->prefix = "$wv";
  while (wvdl_contains(src, (int)len, x->prefix)) {
    x->prefix = wvdl_fmt(x, "%s_", x->prefix);
  }
  wvdl_tpl_id(x);
  root = wvdl_scope_open(x, NULL, 1, NULL);
  wvdl_w1_list(x, prog->list, root, NULL);
  wvdl_w2_list(x, prog->list, root);
  wvdl_plan(x);
  wvdl_lines_init(x);
  memset(&out, 0, sizeof(out));
  out.arena = &x->arena;
  x->out = &out;
  memset(&top, 0, sizeof(top));
  top.kind = FN_TOP;
  top.vars.arena = &x->arena;
  x->fn = &top;
  wvdl_cap_begin(x, &cap);
  wvdl_p_stmts(x, prog->list);
  body = wvdl_cap_end(x, &cap);
  wvdl_p_decls(x, &top);
  wvdl_o(x, body);
  if (out.failed) {
    wvdl_error(x, 0, "out of memory");
  }
  /* the leading space of the first statement */
  body = out.p ? out.p : "";
  if (*body == ' ') {
    body++;
  }
  result = wvdl_copy(body, strlen(body));
  wvdl_arena_free(&x->arena);
  free(x);
  return result;
}
