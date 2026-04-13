#ifndef AHRE_QUICKJS_DISABLED
#include <stdio.h>
#include <quickjs.h>


#include "error.h"
#include "session.h"
#include "js-engine.h"
#include "htmldoc.h"
#include "cmd-out.h"


void jse_clean(JsEngine js[_1_]) {
    if (!js->rt) return;
    JS_RunGC(js->rt);
    JS_FreeContext(js->ctx);
    JS_FreeRuntime(js->rt);
    str_clean(jse_consolebuf(js));
    *js = (JsEngine){0};
}


static inline Err
_jse_set_global_property_str_(JSContext* ctx,const char* name, JSValue value[_1_]) {
    JSValue global = JS_GetGlobalObject(ctx);
    int rv = JS_SetPropertyStr(ctx, global, name, *value);
    JS_FreeValue(ctx, global);
    return rv == -1 /* return -1 on exception */
        ? "error: could not set propery"
        : Ok
        ;
}

/* Console */
static JSClassID console_class_id = 0;
static int init_console_class(JSContext *ctx) {
    JSRuntime* rt = JS_GetRuntime(ctx);
    JS_NewClassID(rt, &console_class_id);
    return JS_NewClass(rt, console_class_id, &(JSClassDef){ "Console", .finalizer=NULL });
}

static JSValue _console_log(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {
    (void)ctx;

    Str* buf = JS_GetOpaque(self, console_class_id);
    if (!buf) return JS_ThrowTypeError(ctx, "js-engine: could not get console");

    for (int i = 0; i < argc; ++i) {

        size_t len;
        const char* arg = JS_ToCStringLen(ctx, &len, argv[i]);
        if (!arg) return JS_ThrowTypeError(ctx, "invalid arg");
        str_append(buf, sv(arg, len));
        JS_FreeCString(ctx, arg);
    }

    return JS_NULL;
}

static Err jse_add_console(JSContext* ctx, HtmlDoc d[_1_]) {

    if (init_console_class(ctx)) return "error: could not initialize console class";

    JSValue console = JS_NewObjectClass(ctx, console_class_id);

    if (JS_IsException(console)) return "error: could not create the console";

    Str* buf = jse_consolebuf(htmldoc_js(d));
    if (JS_SetOpaque(console, buf)) return "error: could not set the console buffer";
    JS_SetPropertyStr(
        ctx,
        console,
        "log",
        JS_NewCFunction(ctx, _console_log, "log", 1)
    );

    try (_jse_set_global_property_str_(ctx, "console", &console));
    return Ok;
}
/**/

/* Html Element */
//TODO: use one class ID for JS Runtime?
static JSClassID element_class_id = 0;

static int init_element_class(JSContext *ctx) {
    JSRuntime* rt = JS_GetRuntime(ctx);
    JS_NewClassID(rt, &element_class_id);
    JSClassDef class_def = { "Element", .finalizer=NULL };
    return JS_NewClass(rt, element_class_id, &class_def);
}


static Err jse_add_document(JSContext* ctx, HtmlDoc d[_1_]) ;
static JSValue
_document_getElementById(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv);

//TODO: move to htmldoc
Err jse_init(HtmlDoc* htmldoc) {

    if (!htmldoc) return "error: no HtmlDoc";

    JsEngine* js = htmldoc_js(htmldoc);

    if (!js) return "error: no JsEngine in HtmlDoc";

    js->rt = JS_NewRuntime();
    if (!js->rt) return "error: could not initialize quickjs runtime";

    js->ctx = JS_NewContext(js->rt);
    if (!js->ctx) {
        JS_FreeRuntime(js->rt);
        return "error: could not initialize quickjs runtime";
    }

    JS_SetContextOpaque(js->ctx, htmldoc);

    Err err = Ok;
    err = jse_add_console(js->ctx, htmldoc);
    ok_then(err, jse_add_document(js->ctx, htmldoc));

    if (err) {
        jse_clean(js);
        return err;
    }
    return Ok;
}


static Err jse_add_document(JSContext* ctx, HtmlDoc d[_1_]) {

    if (init_element_class(ctx)) return "error: could not initialize element class";

    JSValue document = JS_NewObjectClass(ctx, element_class_id);
    if (JS_IsException(document)) return "error: could not create the document";

    if (JS_SetOpaque(document, htmldoc_dom(d).ptr)) return "error: could not set document opaque";
    JS_SetPropertyStr(
        ctx,
        document,
        "getElementById",
        JS_NewCFunction(ctx, _document_getElementById, "getElementById", 1)
    );
    
    JS_SetPropertyStr(ctx, document, "title", JS_NewString(ctx, "Testing QuickJS Document"));
    //JS_SetPropertyStr(ctx, document, "body", JS_NewObject(ctx));
    
    try (_jse_set_global_property_str_(ctx, "document", &document));
    
    return Ok;
}

Err jse_eval(JsEngine js[_1_], Session* s, const char* script, CmdOut* out) {
    (void)s; //TODO: do not pass it anymore
    if (!script) return "error: jse_eval cannot evaluate nullptr";

    JSContext *ctx = jse_context(js);

    if (!ctx) return "no js context (js engine is enabled with .js)";
    
    JSValue result = JS_Eval(ctx, script, strlen(script), NULL, JS_EVAL_TYPE_GLOBAL);
    
    Err err = Ok;

    Str* consolebuf = jse_consolebuf(js);
    if (len__(consolebuf)) {
        try(cmd_out_msg_append_ln(out, consolebuf));
        str_reset(consolebuf);
    }

    if (JS_IsException(result)) {
        JSValue error = JS_GetException(ctx);
        const char *error_str = JS_ToCString(ctx, error);
        err = err_fmt("error evaluating js: %s\n", error_str ? error_str : "(unknown error)");
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, error);
    } else {
        const char* str = JS_ToCString(ctx, result);
        size_t str_len  = strlen(str);
        try(cmd_out_msg_append(out,  svl("  => ")));
        if (str && str_len) try(cmd_out_msg_append_ln(out, str));
        else try(cmd_out_msg_append(out,  svl("\\0")));
        JS_FreeCString(ctx, str);
    }
    
    JS_FreeValue(ctx, result);
    return err;
}


static inline JSValue
_element_get_attribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {
    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");

   DomElem elem = dom_elem_from_ptr(JS_GetOpaque(self, element_class_id));
   if (isnull(elem)) return JS_ThrowTypeError(ctx, "ahrerr: invalid attribute");

    size_t attrlen;
    const char* attr = JS_ToCStringLen(ctx, &attrlen, argv[0]);
    if (!attr) return JS_ThrowTypeError(ctx, "invalid attribute");

    StrView value = dom_elem_attr_value(elem, sv(attr, attrlen));
    if (!value.len || !value.items) return JS_ThrowTypeError(ctx, "invalid attribute");

    return JS_NewStringLen(ctx, (char*)value.items, value.len);
}

static inline JSValue _js_value_from_dom_elem(JSContext *ctx, DomElem elem) {
    JSValue js_element = JS_NewObjectClass(ctx, element_class_id);

    Str* buf = &(Str){0};
    DomAttr attr = dom_elem_first_attr(elem);

    while (!isnull(attr)) {
        if (!dom_attr_has_owner(attr))
            return JS_ThrowTypeError(ctx, "internal error atribute is malformed");

        StrView name  = dom_attr_name_view(attr);
        StrView value = dom_attr_value_view(attr);
        
        str_reset(buf);

        if (str_append_z(buf, &name)) {
            str_clean(buf);
            return JS_ThrowTypeError(ctx, "str append failure");
        }

        if (value.len)
            JS_SetPropertyStr(
                ctx, js_element, items__(buf), JS_NewStringLen(ctx, (char*)value.items, value.len)
            );

        attr = dom_attr_next(attr);
    }
    str_clean(buf);
    if (JS_SetOpaque(js_element, elem.ptr))
        return JS_ThrowTypeError(ctx, "ahrerr: could not set opaque val");

   DomElem e = dom_elem_from_ptr(JS_GetOpaque(js_element, element_class_id));
   if (isnull(e)) return JS_ThrowTypeError(ctx, "ahrerr: invalid attribute");

    JS_SetPropertyStr(
        ctx,
        js_element,
        "getAttribute",
        JS_NewCFunction(ctx, _element_get_attribute, "getAttribute", 1)
    );
    return js_element;
}

static JSValue
_document_getElementById(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {

    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");

    Dom dom = dom_from_ptr(JS_GetOpaque(self, element_class_id));

    if (isnull(dom)) return JS_ThrowTypeError(ctx, "error: document not properly initialized");

    size_t idlen;
    const char *id = JS_ToCStringLen(ctx, &idlen, argv[0]);
    if (!id) return JS_ThrowTypeError(ctx, "Invalid ID");

    DomElem element = dom_get_elem_by_id(dom, sv(id, idlen));
    JS_FreeCString(ctx, id);
    if (isnull(element)) return JS_NULL;

    return _js_value_from_dom_elem(ctx, element);
}
#else /* quickjs disabled: */
typedef int _quickjs_disabled_;
#endif
