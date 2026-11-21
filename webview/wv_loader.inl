/* ---------------------------------------------------------------------------
 * wv_loader.inl - finding WebView2Loader.dll
 *
 * Everything about locating and loading the loader dll: registry lookup,
 * the search path list, the embedded resource, the in-memory mapping.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#define WEBVIEW2_WIN32_DISABLE_AUTO_DETECT "WEBVIEW2_WIN32_DISABLE_AUTO_DETECT"
#define WEBVIEW2_WIN32_PATH "WEBVIEW2_WIN32_PATH"
#define WEBVIEW2_WIN32_USER_DATA_NO_APPDATA "WEBVIEW2_WIN32_USER_DATA_NO_APPDATA"

#define WEBVIEW2_BROWSER_EXECUTABLE_FOLDER "WEBVIEW2_BROWSER_EXECUTABLE_FOLDER"
#define WEBVIEW2_USER_DATA_FOLDER "WEBVIEW2_USER_DATA_FOLDER"

#define WEBVIEW2_RT_UUID "{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
#define EDGE_STABLE_UUID "{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}"

#define KEY_CLIENTS_LOCATION "location"
#define KEY_CLIENTS_PV "pv"
#define KEY_MS_EDGEUPDATE_CLIENTSTATE "SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\ClientState"
#define KEY_MS_EDGEUPDATE_CLIENTS "SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients"

#define WEBVIEW2_LOADER_DLL "WebView2Loader.dll"
#define WEBVIEW2_LOADER_DLL_W L"WebView2Loader.dll"

static int fileExists(LPCSTR lpFileName) {
  return GetFileAttributes(lpFileName) != INVALID_FILE_ATTRIBUTES;
}
static int isDirectory(LPCSTR lpFileName) {
  DWORD attributes = GetFileAttributes(lpFileName);
  return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}
static int isFile(LPCSTR lpFileName) {
  DWORD attributes = GetFileAttributes(lpFileName);
  return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static void findWebView2BrowserExecutableFolder() {
  char data[MAX_PATH];
  char version[64];
  char path[MAX_PATH];
  char *bs;
  DWORD size;
  if ((getenv(WEBVIEW2_BROWSER_EXECUTABLE_FOLDER) != NULL) || (getenv(WEBVIEW2_WIN32_DISABLE_AUTO_DETECT) != NULL)) {
    return;
  }
  /*
  see https://docs.microsoft.com/en-gb/deployedge/microsoft-edge-webview-policies
  see https://docs.microsoft.com/en-gb/microsoft-edge/webview2/concepts/distribution
  see https://docs.microsoft.com/en-gb/microsoft-edge/webview2/reference/win32/webview2-idl?view=webview2-0.9.622#createcorewebview2environmentwithoptions
  To use a fixed version of the WebView2 Runtime, pass the relative path of the folder
  that contains the fixed version of the WebView2 Runtime to browserExecutableFolder.
  The path of fixed version of the WebView2 Runtime should not contain \Edge\Application\.
  When such a path is used, the API will fail with ERROR_NOT_SUPPORTED.
  */
  size = sizeof(data);
  if (webview_reg_read(HKEY_LOCAL_MACHINE, KEY_MS_EDGEUPDATE_CLIENTS "\\" EDGE_STABLE_UUID, KEY_CLIENTS_LOCATION, REG_SZ, &data, &size) == ERROR_SUCCESS) {
    size = sizeof(version);
    if (isDirectory(data) && (webview_reg_read(HKEY_LOCAL_MACHINE, KEY_MS_EDGEUPDATE_CLIENTS "\\" EDGE_STABLE_UUID, KEY_CLIENTS_PV, REG_SZ, &version, &size) == ERROR_SUCCESS)) {
      bs = strrchr(data, '\\');
      if (bs != NULL) {
        *bs = '-';
        sprintf(path, "%s\\%s\\msedge.dll", data, version);
        if (fileExists(path)) {
          sprintf(path, "%s\\%s", data, version);
          webview_print_log("found Edge alternative folder, use " WEBVIEW2_WIN32_DISABLE_AUTO_DETECT " to disable");
          webview_print_log(path);
          SetEnvironmentVariable(WEBVIEW2_BROWSER_EXECUTABLE_FOLDER, path);
        }
      }
    }
  }
}

/* ------------------------------------------------------------------------
 * Locating WebView2Loader.dll
 *
 * The original code did LoadLibrary("WebView2Loader.dll") with a relative
 * name, which only resolves through the default search order (application
 * directory, then the current working directory / PATH). That is why a build
 * works on the developer machine (the dll sits in the build folder, which is
 * also the working directory) and silently falls back to MSHTML/IE anywhere
 * else. We now look in well defined places and report exactly what failed.
 * ------------------------------------------------------------------------ */

#if defined(_WIN64)
#define WEBVIEW2_ARCH_DIR "x64"
#define WEBVIEW2_ARCH_DIR_W L"x64"
#else
#define WEBVIEW2_ARCH_DIR "x86"
#define WEBVIEW2_ARCH_DIR_W L"x86"
#endif

static char webview2_diagnostic[512] = "WebView2: not initialized yet";

static const char *WebView2Diagnostic(void) {
  return webview2_diagnostic;
}

static void webview2_setDiag(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(webview2_diagnostic, sizeof(webview2_diagnostic), format, ap);
  va_end(ap);
  webview_print_log(webview2_diagnostic);
}

static char *webview2_exeDir(char *buffer, size_t size) {
  DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)size);
  char *bs;
  if ((len == 0) || (len >= size)) {
    buffer[0] = '\0';
    return buffer;
  }
  bs = strrchr(buffer, '\\');
  if (bs != NULL) {
    *bs = '\0';
  } else {
    buffer[0] = '\0';
  }
  return buffer;
}

static HMODULE webview2_tryLoadLoader(const char *dir) {
  char path[MAX_PATH * 2];
  HMODULE hModule;
  DWORD err;
  if ((dir == NULL) || (dir[0] == '\0')) {
    /* last resort: default search order (application dir, cwd, PATH) */
    hModule = LoadLibraryA(WEBVIEW2_LOADER_DLL);
    if (hModule != NULL) {
      webview_print_log("loaded " WEBVIEW2_LOADER_DLL " from the search path");
    }
    return hModule;
  }
  if (strlen(dir) + sizeof(WEBVIEW2_LOADER_DLL) + 2 > sizeof(path)) {
    return NULL;
  }
  sprintf(path, "%s\\" WEBVIEW2_LOADER_DLL, dir);
  if (!isFile(path)) {
    return NULL;
  }
  hModule = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hModule == NULL) {
    err = GetLastError();
    if (err == ERROR_BAD_EXE_FORMAT) {
      webview2_setDiag("%s exists but is the wrong architecture, this build "
                       "needs the " WEBVIEW2_ARCH_DIR " dll (error 193)",
                       path);
    } else {
      webview2_setDiag("failed to load %s (error %lu)", path,
                       (unsigned long)err);
    }
  } else {
    webview_print_log("loaded:");
    webview_print_log(path);
  }
  return hModule;
}

/* ------------------------------------------------------------------------
 * Where the embedded dll's bytes come from
 *
 * Two sources, one interface. webview2_loaderBytes() hands back a pointer
 * and a size, and everything downstream is identical either way.
 *
 *   WEBVIEW2_LOADER_ARRAY   a const array compiled in from a header, so
 *                           windres never runs. See wv_loader_data.h.
 *   WEBVIEW2_EMBED_LOADER   an RT_RCDATA resource put there by windres.
 * ---------------------------------------------------------------------- */
#if defined(WEBVIEW2_LOADER_ARRAY)
#include "wv_loader_data.h"
static const void *webview2_loaderBytes(DWORD *size) {
  *size = (DWORD)WebView2Loader_dll_size;
  return (const void *)WebView2Loader_dll;
}
#elif defined(WEBVIEW2_EMBED_LOADER)
#ifndef WEBVIEW2_LOADER_RES_NAME
#define WEBVIEW2_LOADER_RES_NAME "WEBVIEW2LOADER"
#endif
static const void *webview2_loaderBytes(DWORD *size) {
  HMODULE self = GetModuleHandleA(NULL);
  HRSRC res;
  HGLOBAL hRes;
  const void *data;
  *size = 0;
  res = FindResourceA(self, WEBVIEW2_LOADER_RES_NAME, RT_RCDATA);
  if (res == NULL) {
    webview_print_log("no embedded " WEBVIEW2_LOADER_DLL " resource");
    return NULL;
  }
  *size = SizeofResource(self, res);
  hRes = LoadResource(self, res);
  if ((hRes == NULL) || (*size == 0)) {
    return NULL;
  }
  data = LockResource(hRes);
  return data;
}
#endif

#if defined(WEBVIEW2_EMBED_LOADER) || defined(WEBVIEW2_LOADER_ARRAY)
#ifndef WEBVIEW2_LOADER_RES_NAME
#define WEBVIEW2_LOADER_RES_NAME "WEBVIEW2LOADER"
#endif
/* Load WebView2Loader.dll straight out of the executable's own resources.
   The dll is written once to <data root>\runtime\<arch>-<hash>\ (see
   wv_paths.inl) and reused by every app built with this library, so there is
   exactly one copy per distinct dll instead of one per size in %TEMP%. The
   hash is of the bytes, so a new loader version gets its own folder and an
   old one is never overwritten while some other exe is still using it.
   Build with MEMLOAD=1 and nothing is written at all. */
static unsigned long webview2_fnv1a(const unsigned char *p, DWORD n) {
  unsigned long h = 2166136261UL;
  DWORD i;
  for (i = 0; i < n; i++) {
    h ^= p[i];
    h = (h * 16777619UL) & 0xFFFFFFFFUL;
  }
  return h;
}

/* The pre-data-root location. Delete it if it is ours and nothing has it
   loaded; if some old build is running, DeleteFile just fails. */
static void webview2_removeLegacyLoader(DWORD size) {
  char dir[MAX_PATH];
  char path[MAX_PATH * 2];
  if (GetTempPathA(MAX_PATH, dir) == 0) {
    return;
  }
  sprintf(path, "%swebview2-" WEBVIEW2_ARCH_DIR "-%lu", dir,
          (unsigned long)size);
  strcat(path, "\\" WEBVIEW2_LOADER_DLL);
  if (isFile(path) && DeleteFileA(path)) {
    *strrchr(path, '\\') = '\0';
    RemoveDirectoryA(path);
  }
}

static HMODULE webview2_loadLoaderFromResource(void) {
  const void *data;
  DWORD size = 0;
  DWORD written = 0;
  WCHAR dir[WEBVIEW_PATH_MAX];
  WCHAR path[WEBVIEW_PATH_MAX + 64];
  WCHAR part[WEBVIEW_PATH_MAX + 64];
  HANDLE hFile;
  HMODULE hModule;
  DWORD attr;
  data = webview2_loaderBytes(&size);
  if ((data == NULL) || (size == 0)) {
    return NULL;
  }
  if (!webview_path_runtime(dir, WEBVIEW_PATH_MAX)) {
    webview2_setDiag("cannot create the runtime folder for the loader");
    return NULL;
  }
  if (!wvp_fmt(path, WEBVIEW_PATH_MAX + 64,
               L"%ls\\" WEBVIEW2_ARCH_DIR_W L"-%08lx", dir,
               webview2_fnv1a((const unsigned char *)data, size))) {
    return NULL;
  }
  webview_path_mkdirs(path);
  wcscat(path, L"\\" WEBVIEW2_LOADER_DLL_W);
  attr = GetFileAttributesW(path);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    /* write to a private name and rename, so two apps starting at the same
       moment can never load a half written dll */
    if (!wvp_fmt(part, WEBVIEW_PATH_MAX + 64, L"%ls.%lu.part", path,
                 (unsigned long)GetCurrentProcessId())) {
      return NULL;
    }
    hFile = CreateFileW(part, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      webview2_setDiag("cannot extract the embedded loader (error %lu)",
                       (unsigned long)GetLastError());
      return NULL;
    }
    WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);
    if (written != size) {
      DeleteFileW(part);
      return NULL;
    }
    if (!MoveFileExW(part, path, 0)) {
      /* somebody else won the race - theirs is identical, use it */
      DeleteFileW(part);
    }
  }
  hModule = LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hModule != NULL) {
    webview_print_log("loaded the embedded loader from the data root");
    OutputDebugStringW(path);
    webview2_removeLegacyLoader(size);
  }
  return hModule;
}
#endif /* WEBVIEW2_EMBED_LOADER || WEBVIEW2_LOADER_ARRAY */

static HMODULE webview2_findLoader(void) {
  char dir[MAX_PATH * 2];
  char sub[MAX_PATH * 2];
  HMODULE hModule = NULL;
  const char *env = getenv(WEBVIEW2_WIN32_PATH);
  if ((env != NULL) && (strlen(env) < MAX_PATH)) {
    hModule = webview2_tryLoadLoader(env);
    if (hModule != NULL) {
      return hModule;
    }
  }
  webview2_exeDir(dir, sizeof(dir));
  if (dir[0] != '\0') {
    hModule = webview2_tryLoadLoader(dir);
    if (hModule != NULL) {
      return hModule;
    }
    sprintf(sub, "%s\\" WEBVIEW2_ARCH_DIR, dir);
    hModule = webview2_tryLoadLoader(sub);
    if (hModule != NULL) {
      return hModule;
    }
    sprintf(sub, "%s\\ms.webview2\\" WEBVIEW2_ARCH_DIR, dir);
    hModule = webview2_tryLoadLoader(sub);
    if (hModule != NULL) {
      return hModule;
    }
  }
#if defined(WEBVIEW2_EMBED_LOADER) || defined(WEBVIEW2_LOADER_ARRAY)
  hModule = webview2_loadLoaderFromResource();
  if (hModule != NULL) {
    return hModule;
  }
#endif
  return webview2_tryLoadLoader(NULL);
}
