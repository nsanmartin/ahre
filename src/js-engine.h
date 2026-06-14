#ifndef __AHRE_JS_ENGINE_H__
#define __AHRE_JS_ENGINE_H__


typedef struct HtmlDoc HtmlDoc;

#ifdef AHRE_QUICKJS_DISABLED
#define AHRE_QUICKJS_DISABLED_MSG \
    "warn: quickjs not supported in this build. Disable js with \\set session js 0"

typedef int JSRuntime;
typedef int JSContext;
typedef int JsEngine;

/* getters */

static inline bool jse_is_enabled(JsEngine js[_1_]) { (void)js; return 0; }

static inline Err jse_eval(JsEngine js[_1_], Session* s, StrView script, CmdOut* out) {
    (void)js; (void)s; (void)script; (void)out; return AHRE_QUICKJS_DISABLED_MSG;
}

static inline JSRuntime* jse_rt(JsEngine js[_1_]) { (void)js; return 0; }
static inline JSContext* jse_ctx(JsEngine js[_1_]) { (void)js; return 0; }

//TODO: pass htmldoc and evaluate scripts
static inline Err jse_init(Session* s, HtmlDoc* d) { (void)d; return AHRE_QUICKJS_DISABLED_MSG; }

static inline void jse_clean(JsEngine js[_1_]){ (void)js; }

#else /*
     /   quickjs enabled:
   */

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;

typedef enum {
    POST_ACTION_NO_ACTION = 0,
    POST_ACTION_LOCATION_REPLACE,
    POST_ACTION_LOCATION_HREF_SET
} PostAction;

typedef struct {
    JSRuntime  *rt;
    JSContext  *ctx;
    Str        consolebuf;
    PostAction post_action;
} JsEngine;

/* getters */
static inline JSRuntime* jse_runtime(JsEngine js[_1_]) { return js->rt; }
static inline JSContext* jse_context(JsEngine js[_1_]) { return js->ctx; }
static inline Str* jse_consolebuf(JsEngine js[_1_]) { return &js->consolebuf; }

static inline PostAction* jse_post_action(JsEngine js[_1_]) { return &js->post_action; }
static inline bool jse_is_enabled(JsEngine js[_1_]) { return js->rt; }

Err jse_eval(JsEngine js[_1_], Session* s, StrView script, CmdOut* out);

static inline JSRuntime* jse_rt(JsEngine js[_1_]) { return js->rt; }
static inline JSContext* jse_ctx(JsEngine js[_1_]) { return js->ctx; }

//TODO: pass htmldoc and evaluate scripts
Err jse_init(Session* s, HtmlDoc* d);

void jse_clean(JsEngine js[_1_]);

#endif /* AHRE_QUICKJS_DISABLED */
#endif /* __AHRE_JS_ENGINE_H__ */
