/* ---------------------------------------------------------------------------
 * wv_bootstrap.inl - the JavaScript injected into every page
 *
 * Defines window.webview.on / off / send / receive. Written in ES5 so the
 * same string works on WebView2 and on the IE fallback.
 *
 * What your C callback receives, for each way the page can send:
 *
 *   window.external.invoke("hi")        ->  hi              (raw, no quotes)
 *   window.external.invoke({a:1})       ->  {"a":1}         (JSON)
 *   window.webview.send("hi")           ->  hi
 *   window.webview.send({a:1})          ->  {"a":1}
 *   window.chrome.webview.postMessage("hi") -> hi
 *
 * A string always arrives as itself. Anything that is not a string arrives as
 * JSON, because that is the only way to carry it across.
 *
 * Included by webview.h. Not meant to be compiled on its own.
 * -------------------------------------------------------------------------*/

#define WEBVIEW_JS_BOOTSTRAP                                                   \
  "(function(){"                                                               \
  "if(window.webview&&window.webview.__bus)return;"                            \
  "var h={};var api=window.webview||{};api.__bus=1;"                           \
  "api.on=function(n,f){(h[n]=h[n]||[]).push(f);return api;};"                 \
  "api.off=function(n,f){var l=h[n];if(!l)return api;"                         \
  "if(!f){delete h[n];return api;}"                                            \
  "for(var i=l.length-1;i>=0;i--){if(l[i]===f)l.splice(i,1);}return api;};"    \
  "api.receive=function(n,d){var l=(h[n]||[]).slice();"                        \
  "for(var i=0;i<l.length;i++){try{l[i](d);}catch(e){"                         \
  "if(window.console)console.error(e);}}"                                      \
  "try{var ev;if(typeof CustomEvent==='function'){"                            \
  "ev=new CustomEvent('webview:'+n,{detail:d});}else{"                         \
  "ev=document.createEvent('CustomEvent');"                                    \
  "ev.initCustomEvent('webview:'+n,false,false,d);}"                           \
  "window.dispatchEvent(ev);}catch(e2){}};"                                    \
  "if(window.chrome&&window.chrome.webview){"                                  \
  "try{window.external=window.external||{};}catch(e3){}"                       \
  "try{window.external.invoke=function(s){"                                    \
  "window.chrome.webview.postMessage(s);};}catch(e4){}}"                    \
  "api.__pending={};api.__seq=0;"                                              \
  "api.__call=function(name,args){var id=++api.__seq;var msg='__wv:'+id+':'"   \
  "+name.length+':'+name+JSON.stringify(args);"                                \
  "if(typeof Promise==='undefined'){api.send(msg);return;}"                     \
  "return new Promise(function(res,rej){api.__pending[id]={r:res,j:rej};"      \
  "api.send(msg);});};"                                                        \
  "api.__resolve=function(id,status,json){var p=api.__pending[id];"            \
  "if(!p)return;delete api.__pending[id];var v;"                               \
  "try{v=(json===''||json===undefined)?undefined:JSON.parse(json);}"           \
  "catch(e){v=json;}"                                                          \
  "if(status===0)p.r(v);else p.j(v);};"                                        \
  "api.__bind=function(name){var parts=name.split('.');var o=window;var i;"    \
  "for(i=0;i<parts.length-1;i++){"                                             \
  "if(typeof o[parts[i]]!=='object'||o[parts[i]]===null)o[parts[i]]={};"       \
  "o=o[parts[i]];}"                                                            \
  "o[parts[parts.length-1]]=function(){"                                       \
  "return api.__call(name,Array.prototype.slice.call(arguments));};};"         \
  "api.__unbind=function(name){var parts=name.split('.');var o=window;var i;"  \
  "for(i=0;i<parts.length-1;i++){o=o[parts[i]];if(!o)return;}"                 \
  "try{delete o[parts[parts.length-1]];}catch(e){}};"                          \
  "api.send=function(m){var s=(typeof m==='string')?m:JSON.stringify(m);"      \
  "if(window.chrome&&window.chrome.webview){"                                  \
  "window.chrome.webview.postMessage(s);return;}"                              \
  "window.external.invoke(s);};"                                               \
  "window.webview=api;"                                                        \
  "var r=function(){try{api.send('__wvready');}catch(e5){}};"                  \
  "if(document.readyState==='complete'||document.readyState==="                \
  "'interactive'){setTimeout(r,0);}else{"                                      \
  "document.addEventListener('DOMContentLoaded',r);}"                          \
  "})();"
