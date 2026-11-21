/* ===========================================================================
 * wv_value.h - reading JavaScript values from C
 *
 *     #include "webview/webview.h"
 *     #include "webview/wv_value.h"
 *
 * WHAT THIS IS
 *
 * C cannot hold a JavaScript object. What it can hold is JSON, and WebView2
 * gives us exactly that: ICoreWebView2::ExecuteScript hands back the value of
 * the script encoded as JSON. So this module does two things:
 *
 *   1. parses that JSON into a small tree you can walk (wv_value)
 *   2. keeps real JS objects alive on the JS side, addressed from C by an
 *      integer handle (wv_ref), so you can reach into them and call them
 *
 * A wv_value is a snapshot - a copy of what the value looked like at that
 * moment. A wv_ref is a live handle to the real thing. Use a value when you
 * want data, a ref when you want to keep talking to the same object.
 *
 * FUNCTIONS ARE REACHABLE, NOT COPYABLE. You cannot turn a JS function into a
 * C function pointer. You can hold it as a wv_ref and call it: wv_ref_call().
 *
 * THIS IS SYNCHRONOUS, AND THAT COSTS SOMETHING
 *
 * ExecuteScript is asynchronous. To give you a plain `wv_value *v = ...`
 * these functions pump the message loop until the answer arrives. That means:
 *
 *   - only call them from the UI thread;
 *   - NOT from inside external_invoke_cb, or any other WebView2 event
 *     handler: the runtime will not deliver the answer while one of its own
 *     callbacks is still on the stack, so you would wait for the full
 *     timeout and get NULL. Step out of the handler first with
 *     webview_dispatch(), which is what example 06 does;
 *   - the window stays alive and repaints while you wait (good), but your
 *     other message handlers can run re-entrantly (be careful);
 *   - everything takes a timeout, and returns NULL if it expires;
 *   - WebView2 only. On the MSHTML/IE fallback every call returns NULL.
 *
 * If that is not what you want, use webview_eval_result() directly and take
 * the answer in a callback.
 * ===========================================================================
 */

#ifndef WV_VALUE_H
#define WV_VALUE_H

#ifndef WEBVIEW_H
#error "include webview/webview.h before webview/wv_value.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define WV_TIMEOUT_DEFAULT 5000 /* ms */

typedef enum {
  WV_NULL = 0,
  WV_BOOL,
  WV_NUMBER,
  WV_STRING,
  WV_ARRAY,
  WV_OBJECT
} wv_type;

typedef struct wv_value wv_value;

/* A live handle to a JavaScript value kept alive on the JS side.
   0 means "no reference". */
typedef int wv_ref;

/* ---------------------------------------------------------------- parsing */

/* Parse a JSON document. Returns NULL if the text is not valid JSON. */
static wv_value *wv_parse(const char *json);

/* Free a value and everything under it. Passing NULL is fine. */
static void wv_free(wv_value *v);

/* Re-encode as JSON. Caller free()s the result. */
static char *wv_json(const wv_value *v);

/* -------------------------------------------------------------- inspecting */

static wv_type wv_type_of(const wv_value *v);      /* WV_NULL for NULL */
static int wv_is_null(const wv_value *v);
static int wv_bool(const wv_value *v);             /* 0 if not a bool */
static double wv_number(const wv_value *v);        /* 0 if not a number */
static int wv_int(const wv_value *v);
static const char *wv_string(const wv_value *v);   /* NULL if not a string */

/* Arrays and objects. wv_count() is the length of either. */
static int wv_count(const wv_value *v);
static wv_value *wv_at(const wv_value *v, int index);        /* array item */
static wv_value *wv_get(const wv_value *v, const char *key); /* object field */
static const char *wv_key_at(const wv_value *v, int index);  /* field name  */

/* The returned children belong to their parent - do NOT free them, and do
   not use them after the parent has been freed. */

/* ------------------------------------------------------- talking to the page
 *
 * `js` is any expression. "document.title", "1+1", "Object.keys(window)".
 * Returns NULL on timeout, on a script error, or on the IE fallback.
 * The caller free()s the value with wv_free().
 */
static wv_value *wv_eval(struct webview *w, const char *js, int timeout_ms);

/* Convenience: call a global function with JSON arguments.
   wv_call(w, "Math.max", "[3,9,4]", 0) -> 9 */
static wv_value *wv_call(struct webview *w, const char *fn,
                         const char *args_json, int timeout_ms);

/* ------------------------------------------------------------- live handles
 *
 * wv_ref_new(w, "document.body") keeps the real object alive in a JS side
 * table and gives you a handle to it. Release it with wv_ref_free() or the
 * page will hold on to the object forever.
 */
static wv_ref wv_ref_new(struct webview *w, const char *js, int timeout_ms);
static void wv_ref_free(struct webview *w, wv_ref ref);

/* Read `path` inside the referenced object. Path is plain JavaScript that is
   appended to the object, so ".style.color" and "[0]" both work. Pass NULL or
   "" to snapshot the object itself. */
static wv_value *wv_ref_get(struct webview *w, wv_ref ref, const char *path,
                            int timeout_ms);

/* Assign: obj<path> = <json_value>. Returns 0 on success. */
static int wv_ref_set(struct webview *w, wv_ref ref, const char *path,
                      const char *json_value, int timeout_ms);

/* Call a method on the referenced object, or the object itself when `method`
   is NULL (i.e. when the ref is a function). `args_json` is a JSON array. */
static wv_value *wv_ref_call(struct webview *w, wv_ref ref, const char *method,
                             const char *args_json, int timeout_ms);

/* Make a ref out of a sub-value: like wv_ref_new but relative to a ref. */
static wv_ref wv_ref_child(struct webview *w, wv_ref ref, const char *path,
                           int timeout_ms);

#ifdef __cplusplus
}
#endif

#include "wv_value.inl"

#endif /* WV_VALUE_H */
