#ifndef AHRE_QUICKJS_DISABLED
#include <stdio.h>
#include <quickjs.h>

#include <lexbor/html/html.h>

#include "error.h"
#include "session.h"
#include "js-engine.h"
#include "htmldoc.h"


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
        str_append(buf, (char*)arg, len);
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

    lxb_dom_element_t* doc = lxb_dom_interface_element(htmldoc_lxbdoc(d));
    if (JS_SetOpaque(document, doc)) return "error: could not set document opaque";
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

Err jse_eval(JsEngine js[_1_], Session* s, const char* script) {
    if (!script) return "error: jse_eval cannot evaluate nullptr";

    JSContext *ctx = jse_context(js);

    if (!ctx) return "no js context (js engine is enabled with .js)";
    
    JSValue result = JS_Eval(ctx, script, strlen(script), NULL, JS_EVAL_TYPE_GLOBAL);
    
    Err err = Ok;

    Str* consolebuf = jse_consolebuf(js);
    if (len__(consolebuf)) {
        session_write_msg(s, items__(consolebuf), len__(consolebuf));
        session_write_msg_lit__(s, "\n");
        str_reset(consolebuf);
    }

    if (JS_IsException(result)) {
        JSValue error = JS_GetException(ctx);
        const char *error_str = JS_ToCString(ctx, error);
        err = err_fmt("error evaluating js: %s\n", error_str);
        JS_FreeCString(ctx, error_str);
        JS_FreeValue(ctx, error);
    } else {
        const char *str = JS_ToCString(ctx, result);
        session_write_msg_lit__(s, "  => ");
        session_write_msg(s, (char*)str, strlen(str));
        session_write_msg_lit__(s, "\n");
        JS_FreeCString(ctx, str);
    }
    
    JS_FreeValue(ctx, result);
    return err;
}

static inline bool _attr_is_valid_(lxb_dom_attr_t* attr) {
    return attr
        && attr->node.owner_document
        ;
}

static inline JSValue
_element_get_attribute(JSContext *ctx, JSValueConst self, int argc, JSValueConst *argv) {
    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");

   lxb_dom_element_t* element = JS_GetOpaque(self, element_class_id);
   if (!element) return JS_ThrowTypeError(ctx, "ahrerr: invalid attribute");

    size_t attrlen;
    const char* attr = JS_ToCStringLen(ctx, &attrlen, argv[0]);
    if (!attr) return JS_ThrowTypeError(ctx, "invalid attribute");

    size_t valuelen = 0;
    const lxb_char_t * value = lxb_dom_element_get_attribute(
        element, (const lxb_char_t*)attr, attrlen, &valuelen
    );
    if (!valuelen || !value) return JS_ThrowTypeError(ctx, "invalid attribute");

    return JS_NewStringLen(ctx, (char*)value, valuelen);
}

static inline JSValue _js_value_from_lexbor_element(JSContext *ctx, lxb_dom_element_t* element) {
    JSValue js_element = JS_NewObjectClass(ctx, element_class_id);

    Str* buf = &(Str){0};
    lxb_dom_attr_t* attr = lxb_dom_element_first_attribute(element);

    while (attr) {
        if (!_attr_is_valid_(attr))
            return JS_ThrowTypeError(ctx, "internal error atribbute is malformed");

        size_t namelen;
        const lxb_char_t* name = lxb_dom_attr_qualified_name(attr, &namelen);

        size_t valuelen;
        const lxb_char_t* value = lxb_dom_attr_value(attr, &valuelen);
        
        str_reset(buf);

        if (str_append(buf, (char*)name, namelen) || str_append(buf, "\0", 1)) {
            str_clean(buf);
            return JS_ThrowTypeError(ctx, "str append failure");
        }


        if (valuelen)
            JS_SetPropertyStr(
                ctx, js_element, items__(buf), JS_NewStringLen(ctx, (char*)value, valuelen)
            );

        attr = lxb_dom_element_next_attribute(attr);
    }
    str_clean(buf);
    if (JS_SetOpaque(js_element, element))
        return JS_ThrowTypeError(ctx, "aherr: could not set opaque val");

   lxb_dom_element_t* e = JS_GetOpaque(js_element, element_class_id);
   if (!e) return JS_ThrowTypeError(ctx, "ahrerr: invalid attribute");

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

    lxb_dom_element_t* doc = JS_GetOpaque(self, element_class_id);

    if (!doc) return JS_ThrowTypeError(ctx, "error: document not properly initialized");

    size_t idlen;
    const char *id = JS_ToCStringLen(ctx, &idlen, argv[0]);
    if (!id) return JS_ThrowTypeError(ctx, "Invalid ID");

    lxb_dom_element_t* element =
        lexbor_doc_get_element_by_id(lxb_html_interface_document(doc), id, idlen);
    JS_FreeCString(ctx, id);
    if (!element) return JS_NULL;

    return _js_value_from_lexbor_element(ctx, element);
}
#else /* quickjs disabled: */
typedef int _quickjs_disabled_;
#endif
