#ifndef __AHRE_JS_ENGINE_H__
#define __AHRE_JS_ENGINE_H__

typedef struct HtmlDoc HtmlDoc;

#ifndef AHRE_QUICKJS_DISABLED

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;

typedef struct {
    JSRuntime *rt;
    JSContext *ctx;
    Str consolebuf;
} JsEngine;

/* getters */
static inline JSRuntime* jse_runtime(JsEngine js[_1_]) { return js->rt; }
static inline JSContext* jse_context(JsEngine js[_1_]) { return js->ctx; }
static inline Str* jse_consolebuf(JsEngine js[_1_]) { return &js->consolebuf; }

static inline bool jse_is_enabled(JsEngine js[_1_]) { return js->rt; }

Err jse_eval(JsEngine js[_1_], Session* s, const char* script);

static inline JSRuntime* jse_rt(JsEngine js[_1_]) { return js->rt; }
static inline JSContext* jse_ctx(JsEngine js[_1_]) { return js->ctx; }

//TODO: pass htmldoc and evaluate scripts
Err jse_init(HtmlDoc* d);

void jse_clean(JsEngine js[_1_]);

#else /* quickjs disabled: */

#define AHRE_QUICKJS_DISABLED_MSG \
    "warn: quickjs not supported in this build. Disable js with \\set session js 0"

typedef int JSRuntime;
typedef int JSContext;
typedef int JsEngine;

/* getters */

static inline bool jse_is_enabled(JsEngine js[_1_]) { (void)js; return 0; }

static inline Err jse_eval(JsEngine js[_1_], Session* s, const char* script) {
    (void)js; (void)s; (void)script; return AHRE_QUICKJS_DISABLED_MSG;
}

static inline JSRuntime* jse_rt(JsEngine js[_1_]) { (void)js; return 0; }
static inline JSContext* jse_ctx(JsEngine js[_1_]) { (void)js; return 0; }

//TODO: pass htmldoc and evaluate scripts
static inline Err jse_init(HtmlDoc* d) { (void)d; return AHRE_QUICKJS_DISABLED_MSG; }

static inline void jse_clean(JsEngine js[_1_]){ (void)js; }

#endif /* AHRE_QUICKJS_DISABLED */
#endif /* __AHRE_JS_ENGINE_H__ */
