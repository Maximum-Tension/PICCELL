/* ---------------------------------------------------------------------------
 * wv_util.inl - small helpers
 *
 * UTF-8 <-> UTF-16, GUID compare, JSON string escaping, logging.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#ifdef __cplusplus
#define iid_ref(x) &(x)
#define iid_unref(x) *(x)
#else
#define iid_ref(x) (x)
#define iid_unref(x) (x)
#endif

static inline WCHAR *webview_to_utf16(const char *s) {
  DWORD size = MultiByteToWideChar(CP_UTF8, 0, s, -1, 0, 0);
  WCHAR *ws = (WCHAR *)GlobalAlloc(GMEM_FIXED, sizeof(WCHAR) * size);
  if (ws == NULL) {
    return NULL;
  }
  MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, size);
  return ws;
}

static inline char *webview_from_utf16(WCHAR *ws) {
  int n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, NULL, 0, NULL, NULL);
  char *s = (char *)GlobalAlloc(GMEM_FIXED, n);
  if (s == NULL) {
    return NULL;
  }
  WideCharToMultiByte(CP_UTF8, 0, ws, -1, s, n, NULL, NULL);
  return s;
}

static int iid_eq(REFIID a, const IID *b) {
  return memcmp((const void *)iid_ref(a), (const void *)b, sizeof(GUID)) == 0;
}

/* RegGetValueA, by hand: that one is Vista and newer, and an exe importing
   it does not even start on XP. `type` is REG_SZ (the string comes back
   terminated) or REG_DWORD; *size is the room in data, then what was read. */
static LONG webview_reg_read(HKEY root, const char *subkey, const char *name,
                             DWORD type, void *data, DWORD *size) {
  HKEY key;
  DWORD got = 0, room = *size;
  LONG r = RegOpenKeyExA(root, subkey, 0, KEY_READ, &key);
  if (r != ERROR_SUCCESS) {
    return r;
  }
  if ((type == REG_SZ) && (room > 0)) {
    room--; /* keep a byte for the terminator */
  }
  *size = room;
  r = RegQueryValueExA(key, name, NULL, &got, (BYTE *)data, size);
  RegCloseKey(key);
  if (r != ERROR_SUCCESS) {
    return r;
  }
  if (type == REG_SZ) {
    if ((got != REG_SZ) && (got != REG_EXPAND_SZ)) {
      return ERROR_UNSUPPORTED_TYPE;
    }
    ((char *)data)[*size] = 0;
  } else if (got != type) {
    return ERROR_UNSUPPORTED_TYPE;
  }
  return ERROR_SUCCESS;
}

#define DEFAULT_URL                                                            \
  "data:text/"                                                                 \
  "html,%3C%21DOCTYPE%20html%3E%0A%3Chtml%20lang=%22en%22%3E%0A%3Chead%3E%"    \
  "3Cmeta%20charset=%22utf-8%22%3E%3Cmeta%20http-equiv=%22X-UA-Compatible%22%" \
  "20content=%22IE=edge%22%3E%3C%2Fhead%3E%0A%3Cbody%3E%3Cdiv%20id=%22app%22%" \
  "3E%3C%2Fdiv%3E%3Cscript%20type=%22text%2Fjavascript%22%3E%3C%2Fscript%3E%"  \
  "3C%2Fbody%3E%0A%3C%2Fhtml%3E"

#define CSS_INJECT_FUNCTION                                                    \
  "(function(e){var "                                                          \
  "t=document.createElement('style'),d=document.head||document."               \
  "getElementsByTagName('head')[0];t.setAttribute('type','text/"               \
  "css'),t.styleSheet?t.styleSheet.cssText=e:t.appendChild(document."          \
  "createTextNode(e)),d.appendChild(t)})"

WEBVIEW_API char *webview_json_escape(const char *s) {
  size_t n = 3; /* two quotes + terminator */
  size_t i;
  char *out;
  char *p;
  if (s == NULL) {
    s = "";
  }
  for (i = 0; s[i] != '\0'; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t' ||
        c == '\b' || c == '\f') {
      n += 2;
    } else if (c < 0x20) {
      n += 6;
    } else {
      n += 1;
    }
  }
  out = (char *)malloc(n);
  if (out == NULL) {
    return NULL;
  }
  p = out;
  *p++ = '"';
  for (i = 0; s[i] != '\0'; i++) {
    unsigned char c = (unsigned char)s[i];
    switch (c) {
    case '"':
      *p++ = '\\';
      *p++ = '"';
      break;
    case '\\':
      *p++ = '\\';
      *p++ = '\\';
      break;
    case '\n':
      *p++ = '\\';
      *p++ = 'n';
      break;
    case '\r':
      *p++ = '\\';
      *p++ = 'r';
      break;
    case '\t':
      *p++ = '\\';
      *p++ = 't';
      break;
    case '\b':
      *p++ = '\\';
      *p++ = 'b';
      break;
    case '\f':
      *p++ = '\\';
      *p++ = 'f';
      break;
    default:
      if (c < 0x20) {
        sprintf(p, "\\u%04x", (unsigned int)c);
        p += 6;
      } else {
        *p++ = (char)c;
      }
    }
  }
  *p++ = '"';
  *p = '\0';
  return out;
}

/* A file next to the exe, for webview_log_file() and webview_crash_file():
   `name` alone is put beside the executable, a full path is used as is. */
static int webview_beside_exe(char *out, size_t size, const char *name) {
  DWORD len;
  char *slash;
  if ((name == NULL) || (size < MAX_PATH)) {
    return -1;
  }
  if ((strchr(name, '\\') != NULL) || (strchr(name, '/') != NULL) ||
      (strchr(name, ':') != NULL)) {
    snprintf(out, size, "%s", name);
    return 0;
  }
  len = GetModuleFileNameA(NULL, out, (DWORD)size);
  if ((len == 0) || (len >= size)) {
    return -1;
  }
  slash = strrchr(out, '\\');
  if (slash == NULL) {
    return -1;
  }
  snprintf(slash + 1, size - (size_t)(slash + 1 - out), "%s", name);
  return 0;
}

static FILE *webview_log_out = NULL;

/* Also write every log line to a file - started empty. For a -mwindows
   build, which has no console to read them in. */
WEBVIEW_API int webview_log_file(const char *name) {
  char path[MAX_PATH];
  if (webview_beside_exe(path, sizeof(path), name) != 0) {
    return -1;
  }
  if (webview_log_out != NULL) {
    fclose(webview_log_out);
  }
  webview_log_out = fopen(path, "w");
  return (webview_log_out != NULL) ? 0 : -1;
}

/* Goes to the debugger (DebugView, Visual Studio) AND to stderr, so a
   console build shows everything without any extra tooling. A -mwindows
   build has no console, and then the stderr half simply goes nowhere -
   webview_log_file() catches it. */
WEBVIEW_API void webview_print_log(const char *s) {
  OutputDebugStringA(s);
  OutputDebugStringA("\n");
  if (stderr != NULL) {
    fputs(s, stderr);
    fputc('\n', stderr);
    fflush(stderr);
  }
  if (webview_log_out != NULL) {
    fputs(s, webview_log_out);
    fputc('\n', webview_log_out);
    fflush(webview_log_out);
  }
}

/* ------------------------------------------------------------ crash notes
 *
 * For "it just closes": webview_crash_file("crash.txt") makes a crash -
 * an unhandled exception anywhere in the process - leave a note saying
 * what happened, where (module + offset, and the call stack the same way),
 * and the library's last few steps (webview_step) before it. Nothing is
 * written unless the process crashes.
 * -------------------------------------------------------------------------*/
static const char *webview_steps[16];
static unsigned webview_step_count = 0;
static char webview_crash_path[MAX_PATH];

/* a breadcrumb: a string literal naming what is about to happen */
static void webview_step(const char *what) {
  webview_steps[webview_step_count++ & 15] = what;
}

static void webview_crash_where(FILE *f, const void *address) {
  HMODULE module = NULL;
  char name[MAX_PATH];
  const char *base;
  MEMORY_BASIC_INFORMATION mem;
  /* the module an address is in: where its allocation starts (unlike
     GetModuleHandleExA, this is there on Windows 2000 too) */
  if (VirtualQuery(address, &mem, sizeof(mem)) == sizeof(mem)) {
    module = (HMODULE)mem.AllocationBase;
  }
  if ((module != NULL) && (GetModuleFileNameA(module, name, MAX_PATH) > 0)) {
    base = strrchr(name, '\\');
    fprintf(f, "%s+0x%llx", (base != NULL) ? base + 1 : name,
            (unsigned long long)((const char *)address - (const char *)module));
  } else {
    fprintf(f, "%p", address);
  }
}

static LONG WINAPI webview_crash_filter(EXCEPTION_POINTERS *e) {
  FILE *f;
  unsigned i, n;
  if ((e == NULL) || (webview_crash_path[0] == 0)) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  f = fopen(webview_crash_path, "w");
  if (f == NULL) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  fprintf(f, "exception 0x%08lx at ",
          (unsigned long)e->ExceptionRecord->ExceptionCode);
  webview_crash_where(f, e->ExceptionRecord->ExceptionAddress);
  if ((e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) &&
      (e->ExceptionRecord->NumberParameters >= 2)) {
    fprintf(f, ", %s %p",
            e->ExceptionRecord->ExceptionInformation[0] == 0   ? "reading"
            : e->ExceptionRecord->ExceptionInformation[0] == 8 ? "executing"
                                                                : "writing",
            (void *)e->ExceptionRecord->ExceptionInformation[1]);
  }
  fputs("\n\nstack:\n", f);
#if defined(_M_X64) || defined(__x86_64__)
  {
    CONTEXT ctx = *e->ContextRecord;
    for (i = 0; (i < 40) && (ctx.Rip != 0); i++) {
      DWORD64 image = 0, frame = 0;
      PVOID handler = NULL;
      PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(ctx.Rip, &image, NULL);
      fputs("  ", f);
      webview_crash_where(f, (const void *)ctx.Rip);
      fputc('\n', f);
      if (fn == NULL) { /* a leaf: the return address is on top */
        ctx.Rip = *(DWORD64 *)ctx.Rsp;
        ctx.Rsp += 8;
      } else {
        RtlVirtualUnwind(0 /* UNW_FLAG_NHANDLER */, image, ctx.Rip, fn, &ctx,
                         &handler, &frame, NULL);
      }
    }
  }
#else
  {
    /* XP and newer have it; looked up so the exe still starts on 2000 */
    typedef USHORT(WINAPI * CaptureFn)(ULONG, ULONG, PVOID *, PULONG);
    CaptureFn capture = (CaptureFn)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "RtlCaptureStackBackTrace");
    void *frames[40];
    n = (capture != NULL) ? capture(0, 40, frames, NULL) : 0;
    for (i = 0; i < n; i++) {
      fputs("  ", f);
      webview_crash_where(f, frames[i]);
      fputc('\n', f);
    }
  }
#endif
  fputs("\nlast steps, oldest first:\n", f);
  n = (webview_step_count < 16) ? webview_step_count : 16;
  for (i = webview_step_count - n; i != webview_step_count; i++) {
    const char *step = webview_steps[i & 15];
    fprintf(f, "  %s\n", (step != NULL) ? step : "?");
  }
  fclose(f);
  return EXCEPTION_CONTINUE_SEARCH; /* and Windows reports it as usual */
}

WEBVIEW_API int webview_crash_file(const char *name) {
  if (webview_beside_exe(webview_crash_path, sizeof(webview_crash_path),
                         name) != 0) {
    webview_crash_path[0] = 0;
    return -1;
  }
  SetUnhandledExceptionFilter(webview_crash_filter);
  return 0;
}

/* Pass extra command line switches to the browser, e.g. "--disable-gpu" to
   take the GPU compositor out of the picture. The WebView2 runtime reads this
   environment variable itself; it has to be set before the environment is
   created, so call this BEFORE webview_init(). */
WEBVIEW_API int webview_set_browser_arguments(const char *args) {
  char buffer[512];
  if (!SetEnvironmentVariableA("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", args)) {
    return -1;
  }
  snprintf(buffer, sizeof(buffer), "browser arguments: %s",
           (args != NULL) ? args : "(cleared)");
  webview_print_log(buffer);
  return 0;
}

/* Build a file:/// url for a file sitting next to the executable, so an app
   works no matter what the current directory happens to be. Returns buffer,
   or NULL if the path did not fit. */
WEBVIEW_API char *webview_local_url(char *buffer, size_t size,
                                    const char *relative) {
  char path[MAX_PATH];
  char *p;
  DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
  if ((len == 0) || (len >= MAX_PATH)) {
    return NULL;
  }
  p = strrchr(path, '\\');
  if (p == NULL) {
    return NULL;
  }
  p[1] = '\0';
  if (strlen(path) + strlen(relative) + 9 > size) {
    return NULL;
  }
  snprintf(buffer, size, "file:///%s%s", path, relative);
  for (p = buffer; *p != '\0'; p++) {
    if (*p == '\\') {
      *p = '/';
    }
  }
  return buffer;
}

static const char *webview_check_url(const char *url) {
  if (url == NULL || strlen(url) == 0) {
    return DEFAULT_URL;
  }
  return url;
}

WEBVIEW_API void webview_debug(const char *format, ...) {
  char buf[4096];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  webview_print_log(buf);
  va_end(ap);
}

static int webview_js_encode(const char *s, char *esc, size_t n) {
  int r = 1; /* At least one byte for trailing zero */
  for (; *s; s++) {
    const unsigned char c = *s;
    if (c >= 0x20 && c < 0x80 && strchr("<>\\'\"", c) == NULL) {
      if (n > 0) {
        *esc++ = c;
        n--;
      }
      r++;
    } else {
      if (n > 0) {
        snprintf(esc, n, "\\x%02x", (int)c);
        esc += 4;
        n -= 4;
      }
      r += 4;
    }
  }
  return r;
}
