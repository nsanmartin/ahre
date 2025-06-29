#ifndef __AHRE_JS_ENGINE_H__
#define __AHRE_JS_ENGINE_H__

#include <quickjs.h>

typedef struct HtmlDoc HtmlDoc;

typedef lxb_dom_element_t* ElemPtr ;
#define KT ElemPtr
#define VT JSValue
#include "lip.h"

typedef struct {
    HtmlDoc* htmldoc;
} AhreQjsCtx;

typedef struct {
    JSRuntime *rt;
    JSContext *ctx;
    AhreQjsCtx ahqctx;
} JsEngine;

/* getters */
static inline JSRuntime* jse_runtime(JsEngine js[static 1]) { return js->rt; }
static inline JSContext* jse_context(JsEngine js[static 1]) { return js->ctx; }
static inline AhreQjsCtx* jse_ahqctx(JsEngine js[static 1]) { return &js->ahqctx; }

static inline bool jse_is_enabled(JsEngine js[static 1]) { return js->rt; }

Err jse_eval(JsEngine js[static 1], Session* s, const char* script);

static inline JSRuntime* jse_rt(JsEngine js[static 1]) { return js->rt; }
static inline JSContext* jse_ctx(JsEngine js[static 1]) { return js->ctx; }

//TODO: pass htmldoc and evaluate scripts
Err jse_init(HtmlDoc* d);

static inline void jse_clean(JsEngine js[static 1]) {
    JS_RunGC(js->rt);
    JS_FreeContext(js->ctx);
    JS_FreeRuntime(js->rt);
    *js = (JsEngine){0};
}

#endif
