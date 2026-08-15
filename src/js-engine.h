#ifndef __AHRE_JS_ENGINE_H__
#define __AHRE_JS_ENGINE_H__

extern size_t JS_EVAL__ERR_MSG_LEN;
extern char   JS_EVAL__MSGBUF[MAX_MSG_LEN+1];
#define js_eval_err_fmt(Fmt, ...) err_fmt_buf(JS_EVAL__MSGBUF, MAX_MSG_LEN, Fmt,__VA_ARGS__)
bool is_js_eval_err(Err e);

typedef struct HtmlDoc HtmlDoc;


typedef enum {
    POST_ACTION_NO_ACTION = 0,
    POST_ACTION_LOCATION_REPLACE,
    POST_ACTION_LOCATION_HREF_SET,
    POST_ACTION_NO_ACTION_SET_SAME_LOCATION,
    POST_ACTION__MAX__    = POST_ACTION_NO_ACTION_SET_SAME_LOCATION
} PostAction;

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
static inline Err jse_init(Session* s, HtmlDoc* d) {
	(void)d;(void)s;
	return AHRE_QUICKJS_DISABLED_MSG;
}

static inline void jse_clean(JsEngine js[_1_]){ (void)js; }
static inline PostAction jse_get_post_action(JsEngine js[_1_]) { return POST_ACTION_NO_ACTION; }
static inline Err jse_set_post_action(JsEngine js[_1_], PostAction pa) { (void)js; return Ok; }

#else /*
     /   quickjs enabled:
   */

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;

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

static inline bool jse_is_enabled(JsEngine js[_1_]) { return js->rt; }

Err jse_eval(JsEngine js[_1_], Session* s, StrView script, CmdOut* out);

static inline JSRuntime* jse_rt(JsEngine js[_1_]) { return js->rt; }
static inline JSContext* jse_ctx(JsEngine js[_1_]) { return js->ctx; }

//TODO: pass htmldoc and evaluate scripts
Err jse_init(Session* s, HtmlDoc* d);

void jse_clean(JsEngine js[_1_]);
static inline PostAction jse_get_post_action(JsEngine js[_1_]) { return js->post_action; }
Err jse_set_post_action(JsEngine js[_1_], PostAction pa);

#endif /* AHRE_QUICKJS_DISABLED */
#endif /* __AHRE_JS_ENGINE_H__ */
