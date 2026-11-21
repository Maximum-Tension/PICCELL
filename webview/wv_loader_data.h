/* ---------------------------------------------------------------------------
 * wv_loader_data.h - WebView2Loader.dll as a C array
 *
 * The alternative to the .rc route. Build with:
 *
 *     -DWEBVIEW2_LOADER_ARRAY=1 -DWEBVIEW2_MEMORY_MODULE=1
 *
 * and the dll comes from a const array compiled into the exe instead of a
 * resource, so windres never has to run. Same single-file result.
 *
 * The arrays are generated from the dlls by tools/bin2c.c and live in
 * ms.webview2/generated/. Regenerate them if you update the SDK:
 *
 *     gcc tools/bin2c.c -o bin2c
 *     bin2c webview/ms.webview2/x64/WebView2Loader.dll WebView2Loader_dll \
 *         > webview/ms.webview2/generated/loader_x64.h
 *
 * Cost: the array lives in .rdata, so the compiler parses ~500 KB of source
 * and the exe carries the dll uncompressed. The .rc route hands windres the
 * binary directly, which compiles faster. Both produce one self contained
 * exe; pick whichever fits your build.
 *
 * Included by wv_loader.inl. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#if defined(_WIN64) || defined(__x86_64__) || defined(_M_X64)
#include "ms.webview2/generated/loader_x64.h"
#else
#include "ms.webview2/generated/loader_x86.h"
#endif
