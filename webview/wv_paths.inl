/* ---------------------------------------------------------------------------
 * wv_paths.inl - one folder for everything the library writes to disk
 *
 * Before, files ended up in three unrelated places:
 *
 *     %TEMP%\webview2-x64-<size>\WebView2Loader.dll     (one per dll size)
 *     %APPDATA%\<exe name>.WebView2\                      (one per EXE NAME)
 *     wherever WEBVIEW2_USER_DATA_FOLDER pointed
 *
 * Now there is one root, and everything lives under it:
 *
 *     %LOCALAPPDATA%\webview-c\
 *       runtime\
 *         x64-<hash>\WebView2Loader.dll     only if the dll has to be
 *                                           extracted (not with MEMLOAD=1)
 *       projects\
 *         <project>\                        WebView2 user data folder:
 *           EBWebView\Default\              cookies, localStorage, cache...
 *           EBWebView\<partition>\          one per <webview partition="">
 *
 * <project> is what you pass to webview_set_app_name(), or the exe name
 * without ".exe" when you pass nothing. Call webview_set_app_name() and every
 * build of your app - app.exe, app_debug.exe, test2.exe - shares one folder.
 *
 * Why LOCALAPPDATA and not TEMP: cookies and localStorage are data people
 * expect to survive, and Storage Sense empties TEMP whenever it likes. Why not
 * APPDATA (Roaming): on domain machines Roaming is synced at logon, and a
 * browser cache is hundreds of megabytes nobody wants synced. LOCALAPPDATA is
 * also what Microsoft recommends for WebView2 user data.
 *
 * Overrides, strongest first:
 *   WEBVIEW2_USER_DATA_FOLDER            WebView2 itself uses that folder
 *   WEBVIEW2_WIN32_USER_DATA_NO_APPDATA  let WebView2 pick (next to the exe)
 *   webview_set_data_root("D:\\x")       move the whole root
 *   WEBVIEW_DATA_ROOT                    same, from the environment
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#ifndef WEBVIEW_DATA_ROOT_NAME
#define WEBVIEW_DATA_ROOT_NAME L"webview-c"
#endif

#define WEBVIEW_PATH_MAX 1024

/* swprintf that reports truncation. MinGW's swprintf returns -1 and leaves
   the buffer unterminated when the text does not fit; this always
   terminates, and returns 1 only if everything fit. */
static int wvp_fmt(WCHAR *out, size_t size, const WCHAR *fmt, ...) {
  va_list ap;
  int n;
  if ((out == NULL) || (size == 0)) {
    return 0;
  }
  va_start(ap, fmt);
  n = _vsnwprintf(out, size, fmt, ap);
  va_end(ap);
  if ((n < 0) || ((size_t)n >= size)) {
    out[size - 1] = L'\0';
    return 0;
  }
  return 1;
}

/* The project name, see webview_set_app_name(). Empty = derive from the exe. */
static WCHAR webview_app_name[MAX_PATH] = {0};
/* An explicit root from webview_set_data_root(). Empty = default. */
static WCHAR webview_data_root_override[WEBVIEW_PATH_MAX] = {0};

/* Share one profile between builds. Pass a stable name for your app and
   every exe using that name reuses the same folder instead of accumulating
   one each. Pass NULL to go back to naming it after the exe.
   Call it before webview_init(). */
WEBVIEW_API void webview_set_app_name(const char *name) {
  if ((name == NULL) || (name[0] == '\0')) {
    webview_app_name[0] = L'\0';
    return;
  }
  MultiByteToWideChar(CP_UTF8, 0, name, -1, webview_app_name, MAX_PATH);
  webview_app_name[MAX_PATH - 1] = L'\0';
}

/* Move the whole data root somewhere else (UTF-8 path). NULL restores the
   default, %LOCALAPPDATA%\webview-c. Call it before webview_init(). */
WEBVIEW_API void webview_set_data_root(const char *path) {
  if ((path == NULL) || (path[0] == '\0')) {
    webview_data_root_override[0] = L'\0';
    return;
  }
  MultiByteToWideChar(CP_UTF8, 0, path, -1, webview_data_root_override,
                      WEBVIEW_PATH_MAX);
  webview_data_root_override[WEBVIEW_PATH_MAX - 1] = L'\0';
}

/* Characters Windows does not allow in a file name become '_', and so do
   trailing dots and spaces, which Explorer cannot delete. */
static void webview_path_sanitize(WCHAR *s) {
  size_t n;
  WCHAR *p;
  for (p = s; *p != L'\0'; p++) {
    if ((*p < 0x20) || (wcschr(L"<>:\"/\\|?*", *p) != NULL)) {
      *p = L'_';
    }
  }
  n = wcslen(s);
  while ((n > 0) && ((s[n - 1] == L'.') || (s[n - 1] == L' '))) {
    s[--n] = L'\0';
  }
  if (n == 0) {
    wcscpy(s, L"app");
  }
}

/* mkdir -p. Returns 1 if the directory exists afterwards. */
static int webview_path_mkdirs(const WCHAR *dir) {
  WCHAR tmp[WEBVIEW_PATH_MAX];
  WCHAR *p;
  DWORD attr;
  if (wcslen(dir) >= WEBVIEW_PATH_MAX) {
    return 0;
  }
  wcscpy(tmp, dir);
  /* skip the drive ("C:\") or the server share ("\\server\share\") */
  p = tmp;
  if ((p[0] == L'\\') && (p[1] == L'\\')) {
    p = wcschr(p + 2, L'\\');
    if (p != NULL) {
      p = wcschr(p + 1, L'\\');
    }
  } else if ((p[0] != L'\0') && (p[1] == L':')) {
    p = wcschr(p, L'\\');
  }
  while ((p != NULL) && (*p != L'\0')) {
    p = wcschr(p + 1, L'\\');
    if (p == NULL) {
      break;
    }
    *p = L'\0';
    CreateDirectoryW(tmp, NULL);
    *p = L'\\';
  }
  CreateDirectoryW(tmp, NULL);
  attr = GetFileAttributesW(tmp);
  return (attr != INVALID_FILE_ATTRIBUTES) &&
         (attr & FILE_ATTRIBUTE_DIRECTORY);
}

/* %LOCALAPPDATA%\webview-c, or the override. Not created. */
static int webview_path_root(WCHAR *out, size_t size) {
  WCHAR env[WEBVIEW_PATH_MAX];
  DWORD n;
  if (webview_data_root_override[0] != L'\0') {
    return wvp_fmt(out, size, L"%ls", webview_data_root_override);
  }
  n = GetEnvironmentVariableW(L"WEBVIEW_DATA_ROOT", env, WEBVIEW_PATH_MAX);
  if ((n > 0) && (n < WEBVIEW_PATH_MAX)) {
    return wvp_fmt(out, size, L"%ls", env);
  }
  n = GetEnvironmentVariableW(L"LOCALAPPDATA", env, WEBVIEW_PATH_MAX);
  if ((n == 0) || (n >= WEBVIEW_PATH_MAX)) {
    /* very old or very locked down: fall back to TEMP rather than failing */
    n = GetTempPathW(WEBVIEW_PATH_MAX, env);
    if ((n == 0) || (n >= WEBVIEW_PATH_MAX)) {
      return 0;
    }
  }
  n = (DWORD)wcslen(env);
  while ((n > 0) && (env[n - 1] == L'\\')) {
    env[--n] = L'\0';
  }
  return wvp_fmt(out, size, L"%ls\\" WEBVIEW_DATA_ROOT_NAME, env);
}

/* The project name: webview_set_app_name(), or the exe name minus ".exe". */
static void webview_path_project_name(WCHAR *out, size_t size) {
  WCHAR exe[MAX_PATH];
  WCHAR *name;
  WCHAR *dot;
  if (webview_app_name[0] != L'\0') {
    wvp_fmt(out, size, L"%ls", webview_app_name);
  } else {
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    exe[MAX_PATH - 1] = L'\0';
    name = wcsrchr(exe, L'\\');
    name = (name != NULL) ? name + 1 : exe;
    dot = wcsrchr(name, L'.');
    if ((dot != NULL) && (_wcsicmp(dot, L".exe") == 0)) {
      *dot = L'\0';
    }
    wvp_fmt(out, size, L"%ls", name);
  }
  webview_path_sanitize(out);
}

/* <root>\projects\<project>, created. */
static int webview_path_project(WCHAR *out, size_t size) {
  WCHAR root[WEBVIEW_PATH_MAX];
  WCHAR project[MAX_PATH];
  if (!webview_path_root(root, WEBVIEW_PATH_MAX)) {
    return 0;
  }
  webview_path_project_name(project, MAX_PATH);
  if (!wvp_fmt(out, size, L"%ls\\projects\\%ls", root, project)) {
    return 0;
  }
  return webview_path_mkdirs(out);
}

/* <root>\runtime, created. */
static int webview_path_runtime(WCHAR *out, size_t size) {
  WCHAR root[WEBVIEW_PATH_MAX];
  if (!webview_path_root(root, WEBVIEW_PATH_MAX)) {
    return 0;
  }
  if (!wvp_fmt(out, size, L"%ls\\runtime", root)) {
    return 0;
  }
  return webview_path_mkdirs(out);
}

/* The old layout put the profile in %APPDATA%\<exe>.WebView2. If that folder
   exists and the new one has no profile yet, move it over once so nobody is
   logged out by upgrading. A plain rename, so it is instant when both are on
   the same drive; if it fails (other drive, files in use) the old folder is
   left exactly where it was and we just start fresh. */
static void webview_path_migrate_legacy(const WCHAR *project_dir) {
  WCHAR appData[WEBVIEW_PATH_MAX];
  WCHAR legacy[WEBVIEW_PATH_MAX];
  WCHAR target[WEBVIEW_PATH_MAX];
  WCHAR exe[MAX_PATH];
  WCHAR *name;
  DWORD n;
  n = GetEnvironmentVariableW(L"APPDATA", appData, WEBVIEW_PATH_MAX);
  if ((n == 0) || (n >= WEBVIEW_PATH_MAX)) {
    return;
  }
  if (webview_app_name[0] != L'\0') {
    name = webview_app_name;
  } else {
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    exe[MAX_PATH - 1] = L'\0';
    name = wcsrchr(exe, L'\\');
    name = (name != NULL) ? name + 1 : exe;
  }
  if (!wvp_fmt(legacy, WEBVIEW_PATH_MAX, L"%ls\\%ls.WebView2\\EBWebView",
               appData, name) ||
      !wvp_fmt(target, WEBVIEW_PATH_MAX, L"%ls\\EBWebView", project_dir)) {
    return;
  }
  if ((GetFileAttributesW(legacy) == INVALID_FILE_ATTRIBUTES) ||
      (GetFileAttributesW(target) != INVALID_FILE_ATTRIBUTES)) {
    return;
  }
  if (MoveFileExW(legacy, target, 0)) {
    /* the now empty <exe>.WebView2 shell */
    if (wvp_fmt(legacy, WEBVIEW_PATH_MAX, L"%ls\\%ls.WebView2", appData, name)) {
      RemoveDirectoryW(legacy);
    }
    webview_print_log("moved the old %APPDATA% profile into the data root");
  } else {
    webview_print_log("old %APPDATA% profile left in place (could not move)");
  }
}

/* The folder handed to CreateCoreWebView2EnvironmentWithOptions, or NULL to
   let WebView2 decide (when one of the environment overrides is set). */
static WCHAR *getUserData(WCHAR *buffer, size_t sizeOfBuffer) {
  if ((getenv("WEBVIEW2_USER_DATA_FOLDER") != NULL) ||
      (getenv("WEBVIEW2_WIN32_USER_DATA_NO_APPDATA") != NULL)) {
    return NULL;
  }
  if (!webview_path_project(buffer, sizeOfBuffer)) {
    webview_print_log("could not create the data folder, WebView2 picks one");
    return NULL;
  }
  webview_path_migrate_legacy(buffer);
  webview_print_log("user data folder:");
  OutputDebugStringW(buffer);
  if (stderr != NULL) {
    fprintf(stderr, "  %ls\n", buffer);
  }
  return buffer;
}

/* ------------------------------------------------------------------------
 * Public helpers
 * ---------------------------------------------------------------------- */

/* UTF-8 path of <root>\projects\<project>\<sub>, created on the way. Handy
   for your own files (settings, exports) so they sit next to the profile.
   `sub` may be NULL or "" for the project folder itself, and may contain
   backslashes. Returns buffer, or NULL on failure. */
WEBVIEW_API char *webview_data_dir(char *buffer, size_t size,
                                   const char *sub) {
  WCHAR project[WEBVIEW_PATH_MAX];
  WCHAR wsub[MAX_PATH];
  WCHAR full[WEBVIEW_PATH_MAX];
  if ((buffer == NULL) || (size == 0)) {
    return NULL;
  }
  if (!webview_path_project(project, WEBVIEW_PATH_MAX)) {
    return NULL;
  }
  if ((sub != NULL) && (sub[0] != '\0')) {
    WCHAR *p;
    MultiByteToWideChar(CP_UTF8, 0, sub, -1, wsub, MAX_PATH);
    wsub[MAX_PATH - 1] = L'\0';
    for (p = wsub; *p != L'\0'; p++) {
      if (*p == L'/') {
        *p = L'\\';
      }
    }
    if (wcsstr(wsub, L"..") != NULL) {
      return NULL; /* stay inside the project */
    }
    if (!wvp_fmt(full, WEBVIEW_PATH_MAX, L"%ls\\%ls", project, wsub) ||
        !webview_path_mkdirs(full)) {
      return NULL;
    }
  } else {
    wcscpy(full, project);
  }
  if (WideCharToMultiByte(CP_UTF8, 0, full, -1, buffer, (int)size, NULL,
                          NULL) == 0) {
    return NULL;
  }
  return buffer;
}

/* Delete a project's whole data folder - cookies, storage, cache, every
   partition. `project` NULL means this app's own project. Only works while no
   browser is using it: call it before webview_init() or after webview_exit().
   Returns 0 on success, -1 if anything was left behind. */
WEBVIEW_API int webview_data_clear(const char *project) {
  WCHAR root[WEBVIEW_PATH_MAX];
  WCHAR name[MAX_PATH];
  WCHAR path[WEBVIEW_PATH_MAX + 2];
  SHFILEOPSTRUCTW op;
  size_t n;
  if (!webview_path_root(root, WEBVIEW_PATH_MAX)) {
    return -1;
  }
  if ((project != NULL) && (project[0] != '\0')) {
    MultiByteToWideChar(CP_UTF8, 0, project, -1, name, MAX_PATH);
    name[MAX_PATH - 1] = L'\0';
    webview_path_sanitize(name);
  } else {
    webview_path_project_name(name, MAX_PATH);
  }
  if (!wvp_fmt(path, WEBVIEW_PATH_MAX, L"%ls\\projects\\%ls", root, name)) {
    return -1;
  }
  n = wcslen(path);
  if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }
  path[n + 1] = L'\0'; /* SHFileOperation wants a double terminator */
  ZeroMemory(&op, sizeof(op));
  op.wFunc = FO_DELETE;
  op.pFrom = path;
  op.fFlags = FOF_NO_UI;
  if ((SHFileOperationW(&op) != 0) || op.fAnyOperationsAborted) {
    return -1;
  }
  return 0;
}
