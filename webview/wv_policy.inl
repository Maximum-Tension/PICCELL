/* ---------------------------------------------------------------------------
 * wv_policy.inl - which backend to use
 *
 * Windows version detection and the WEBVIEW_BACKEND_* policy.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------
 * Backend policy
 *
 * WebView2 support for Windows 7/8/8.1 ended with runtime 109 (early 2024),
 * so on those systems the MSHTML/IE control is the only thing guaranteed to
 * be present. WEBVIEW_BACKEND_AUTO therefore falls back silently there, and
 * only insists on WebView2 where WebView2 can actually be installed.
 *
 * Move the line by defining these before including webview.h. 6.2 means
 * "Windows 8 and newer are modern"; use 6.4 to treat 8 and 8.1 as legacy too
 * (they lost WebView2 support at the same time as 7).
 * ------------------------------------------------------------------------ */
#ifndef WEBVIEW_LEGACY_BELOW_MAJOR
#define WEBVIEW_LEGACY_BELOW_MAJOR 6
#endif
#ifndef WEBVIEW_LEGACY_BELOW_MINOR
#define WEBVIEW_LEGACY_BELOW_MINOR 2
#endif

WEBVIEW_API void webview_windows_version(int *major, int *minor, int *build) {
  /* GetVersionEx lies once an application manifest is present: it reports
     the highest OS the manifest claims to support. RtlGetVersion does not. */
  typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW *);
  OSVERSIONINFOW vi;
  HMODULE ntdll;
  RtlGetVersionFn fn;
  ZeroMemory(&vi, sizeof(vi));
  vi.dwOSVersionInfoSize = sizeof(vi);
  ntdll = GetModuleHandleA("ntdll.dll");
  if (ntdll != NULL) {
    fn = (RtlGetVersionFn)GetProcAddress(ntdll, "RtlGetVersion");
    if (fn != NULL) {
      fn(&vi);
    }
  }
  if (major != NULL) {
    *major = (int)vi.dwMajorVersion;
  }
  if (minor != NULL) {
    *minor = (int)vi.dwMinorVersion;
  }
  if (build != NULL) {
    *build = (int)vi.dwBuildNumber;
  }
}

WEBVIEW_API int webview_is_legacy_windows(void) {
  int major = 0;
  int minor = 0;
  webview_windows_version(&major, &minor, NULL);
  if (major == 0) {
    return 0; /* could not tell, assume modern */
  }
  if (major != WEBVIEW_LEGACY_BELOW_MAJOR) {
    return major < WEBVIEW_LEGACY_BELOW_MAJOR;
  }
  return minor < WEBVIEW_LEGACY_BELOW_MINOR;
}

static int webview_policy = WEBVIEW_BACKEND_AUTO;

WEBVIEW_API void webview_set_backend_policy(int policy) {
  webview_policy = policy;
}

WEBVIEW_API void webview_set_ie_fallback(int allow) {
  webview_policy = allow ? WEBVIEW_BACKEND_ALLOW_IE : WEBVIEW_BACKEND_WEBVIEW2;
}

/* Resolve AUTO and the environment overrides into a concrete policy. */
static int webview_resolve_policy(void) {
  int policy = webview_policy;
  const char *env = getenv("WEBVIEW_BACKEND");
  if (env != NULL) {
    if (_stricmp(env, "ie") == 0 || _stricmp(env, "mshtml") == 0) {
      policy = WEBVIEW_BACKEND_IE;
    } else if (_stricmp(env, "webview2") == 0 || _stricmp(env, "edge") == 0) {
      policy = WEBVIEW_BACKEND_WEBVIEW2;
    } else if (_stricmp(env, "allow-ie") == 0) {
      policy = WEBVIEW_BACKEND_ALLOW_IE;
    } else if (_stricmp(env, "auto") == 0) {
      policy = WEBVIEW_BACKEND_AUTO;
    }
  }
  if (getenv("WEBVIEW_ALLOW_IE") != NULL) {
    policy = WEBVIEW_BACKEND_ALLOW_IE;
  }
  if (getenv("WEBVIEW_DISABLE_WEBVIEW2") != NULL) {
    policy = WEBVIEW_BACKEND_IE;
  }
  if (policy == WEBVIEW_BACKEND_AUTO) {
    policy = webview_is_legacy_windows() ? WEBVIEW_BACKEND_ALLOW_IE
                                         : WEBVIEW_BACKEND_WEBVIEW2;
  }
  return policy;
}

WEBVIEW_API int webview_backend(struct webview *w) {
  return webview_webview2_enabled ? 1 : 0;
}

WEBVIEW_API const char *webview_backend_info(void) {
  static char info[768];
  int major = 0;
  int minor = 0;
  int build = 0;
  webview_windows_version(&major, &minor, &build);
  snprintf(info, sizeof(info), "backend: %s | Windows %d.%d build %d%s | %s",
           webview_webview2_enabled ? "WebView2" : "MSHTML/IE", major, minor,
           build, webview_is_legacy_windows() ? " (legacy)" : "",
           WebView2Diagnostic());
  return info;
}

#define WEBVIEW2_DOWNLOAD_URL                                                  \
  "https://developer.microsoft.com/microsoft-edge/webview2/"

static void webview_report_no_webview2(void) {
  char message[1024];
  snprintf(message, sizeof(message),
           "This application needs the Microsoft Edge WebView2 runtime.\n\n"
           "%s\n\nOpen the Microsoft download page now?",
           webview_backend_info());
  if (MessageBoxA(NULL, message, "WebView2 required",
                  MB_YESNO | MB_ICONERROR) == IDYES) {
    ShellExecuteA(NULL, "open", WEBVIEW2_DOWNLOAD_URL, NULL, NULL, SW_SHOWNORMAL);
  }
}
