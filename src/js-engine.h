#ifndef __AHRE_JS_ENGINE_H__
#define __AHRE_JS_ENGINE_H__

#include <quickjs.h>

typedef struct {
    JSRuntime *rt;
    JSContext *ctx;
} JsEngine;

/* getters */
static inline JSRuntime* jse_runtime(JsEngine js[static 1]) { return js->rt; }
static inline JSContext* jse_context(JsEngine js[static 1]) { return js->ctx; }

Err jse_add_document(JsEngine jse[static 1]);
Err jse_eval(JsEngine js[static 1], Session* s, const char* script);


static inline JSRuntime* jse_rt(JsEngine js[static 1]) { return js->rt; }
static inline JSContext* jse_ctx(JsEngine js[static 1]) { return js->ctx; }

static inline Err jse_init(JsEngine js[static 1]) {
    js->rt = JS_NewRuntime();
    if (!js->rt) return "error: could not initialize quickjs runtime";
    js->ctx = JS_NewContext(js->rt);
    if (!js->ctx) {
        JS_FreeRuntime(js->rt);
        return "error: could not initialize quickjs runtime";
    }
    try( jse_add_document(js));
    return Ok;
}

static inline void jse_clean(JsEngine js[static 1]) {
    JS_FreeContext(js->ctx);
    JS_FreeRuntime(js->rt);
    *js = (JsEngine){0};
}

#endif
