/* ---------------------------------------------------------------------------
 * wv_browser.inl - browser settings as separate toggles
 *
 * The other half of w.window: things that belong to the page rather than the
 * window. Same idea, same shape.
 *
 *     w.browser.context_menu = WEBVIEW_OFF;   /. no right-click menu     ./
 *     w.browser.devtools     = WEBVIEW_ON;    /. F12 even without debug  ./
 *     w.browser.zoom_control = WEBVIEW_OFF;   /. no ctrl+wheel zooming   ./
 *
 * Every field is tri-state and 0 is "leave it alone", so a memset opts out of
 * all of them and you get WebView2's own defaults:
 *
 *     WEBVIEW_DEFAULT (0)   whatever WebView2 does normally
 *     WEBVIEW_ON      (1)   force on
 *     WEBVIEW_OFF    (-1)   force off
 *
 * Two exceptions. devtools follows w.debug when you leave it at
 * WEBVIEW_DEFAULT, so existing code keeps working. And html_fullscreen is on
 * at WEBVIEW_DEFAULT: a page calling element.requestFullscreen() takes the
 * window fullscreen, as in any browser, where WebView2 on its own would only
 * stretch the element over the window it already has. WEBVIEW_OFF gives you
 * WebView2's behaviour back. It is read whenever a page asks, so there is
 * nothing to apply - see webview_html_fullscreen_changed in wv_options.inl.
 *
 * Applied when the browser is created and re-applied whenever you change one,
 * so these are all safe to set before webview_init() or after it.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

/* Resolve a tri-state against a fallback, and skip the call entirely when
   there is nothing to say. Returns 1 to apply, 0 to leave untouched. */
static int webview_tristate(int value, int fallback, BOOL *out) {
  if (value == WEBVIEW_ON) {
    *out = TRUE;
    return 1;
  }
  if (value == WEBVIEW_OFF) {
    *out = FALSE;
    return 1;
  }
  if (fallback >= 0) {
    *out = fallback ? TRUE : FALSE;
    return 1;
  }
  return 0;
}

WEBVIEW_API int webview_apply_browser_options(struct webview *w) {
  ICoreWebView2Settings *settings = NULL;
  ICoreWebView2Settings3 *settings3 = NULL;
  ICoreWebView2Settings6 *settings6 = NULL;
  BOOL value;

  if ((w == NULL) || !webview_webview2_enabled ||
      (w->priv.webview2 == NULL) || (w->priv.webview2->webview == NULL)) {
    return -1;
  }
  if (FAILED(w->priv.webview2->webview->lpVtbl->get_Settings(
          w->priv.webview2->webview, &settings)) ||
      (settings == NULL) || (settings->lpVtbl == NULL)) {
    return -1;
  }

  /* Right-click menu, the one with Inspect on it. WebView2 shows it by
     default, so leaving this at WEBVIEW_DEFAULT gets you the normal menu. */
  if (webview_tristate(w->browser.context_menu, -1, &value)) {
    settings->lpVtbl->put_AreDefaultContextMenusEnabled(settings, value);
  }
  /* Devtools falls back to w.debug so old code behaves as it always did. */
  if (webview_tristate(w->browser.devtools, w->debug, &value)) {
    settings->lpVtbl->put_AreDevToolsEnabled(settings, value);
  }
  /* The little URL popup in the bottom left on hover. */
  if (webview_tristate(w->browser.status_bar, -1, &value)) {
    settings->lpVtbl->put_IsStatusBarEnabled(settings, value);
  }
  /* ctrl+wheel and ctrl +/-. Worth turning off for a fixed-layout app. */
  if (webview_tristate(w->browser.zoom_control, -1, &value)) {
    settings->lpVtbl->put_IsZoomControlEnabled(settings, value);
  }
  /* alert(), confirm(), prompt(). Off means they silently do nothing. */
  if (webview_tristate(w->browser.script_dialogs, -1, &value)) {
    settings->lpVtbl->put_AreDefaultScriptDialogsEnabled(settings, value);
  }
  /* Chromium's own error page when navigation fails. */
  if (webview_tristate(w->browser.error_page, -1, &value)) {
    settings->lpVtbl->put_IsBuiltInErrorPageEnabled(settings, value);
  }
  if (webview_tristate(w->browser.script, -1, &value)) {
    settings->lpVtbl->put_IsScriptEnabled(settings, value);
  }

  /* Browser shortcuts: F5, ctrl+P, ctrl+R, ctrl+F and friends. An app window
     usually wants these gone. Needs a runtime with Settings3. */
  if (w->browser.accelerator_keys != WEBVIEW_DEFAULT) {
    if (SUCCEEDED(settings->lpVtbl->QueryInterface(
            settings, iid_unref(&IID_ICoreWebView2Settings3),
            (void **)&settings3)) &&
        (settings3 != NULL)) {
      webview_tristate(w->browser.accelerator_keys, -1, &value);
      settings3->lpVtbl->put_AreBrowserAcceleratorKeysEnabled(settings3, value);
      settings3->lpVtbl->Release(settings3);
    }
  }
  /* Two-finger / touch swipe going back and forward. Needs Settings6. */
  if (w->browser.swipe_navigation != WEBVIEW_DEFAULT) {
    if (SUCCEEDED(settings->lpVtbl->QueryInterface(
            settings, iid_unref(&IID_ICoreWebView2Settings6),
            (void **)&settings6)) &&
        (settings6 != NULL)) {
      webview_tristate(w->browser.swipe_navigation, -1, &value);
      settings6->lpVtbl->put_IsSwipeNavigationEnabled(settings6, value);
      settings6->lpVtbl->Release(settings6);
    }
  }
  return 0;
}

/* --------------------------------------------------------------- setters */

WEBVIEW_API void webview_browser_set_context_menu(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->browser.context_menu = on ? WEBVIEW_ON : WEBVIEW_OFF;
  webview_apply_browser_options(w);
}

WEBVIEW_API void webview_browser_set_devtools(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->browser.devtools = on ? WEBVIEW_ON : WEBVIEW_OFF;
  webview_apply_browser_options(w);
}

/* Opens the devtools now, whatever w.browser.devtools says: WebView2's own
   window, or on the IE backend the console and inspector drawn into the
   page (F12 there too, when devtools are on). */
WEBVIEW_API void webview_open_devtools(struct webview *w) {
  if (w == NULL) {
    return;
  }
  if (webview_webview2_enabled) {
    if ((w->priv.webview2 != NULL) && (w->priv.webview2->webview != NULL)) {
      w->priv.webview2->webview->lpVtbl->OpenDevToolsWindow(
          w->priv.webview2->webview);
    }
  } else {
    webview_ie_devtools(w, 0);
  }
}

WEBVIEW_API void webview_browser_set_zoom_control(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->browser.zoom_control = on ? WEBVIEW_ON : WEBVIEW_OFF;
  webview_apply_browser_options(w);
}

WEBVIEW_API void webview_browser_set_status_bar(struct webview *w, int on) {
  if (w == NULL) { return; }
  w->browser.status_bar = on ? WEBVIEW_ON : WEBVIEW_OFF;
  webview_apply_browser_options(w);
}

WEBVIEW_API void webview_browser_set_accelerator_keys(struct webview *w,
                                                      int on) {
  if (w == NULL) { return; }
  w->browser.accelerator_keys = on ? WEBVIEW_ON : WEBVIEW_OFF;
  webview_apply_browser_options(w);
}
