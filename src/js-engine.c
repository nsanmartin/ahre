#include <stdio.h>

#include <lexbor/html/html.h>

#include "error.h"
#include "session.h"
#include "js-engine.h"
#include "htmldoc.h"


static inline Err ahqctx_init(AhreQjsCtx ctx[static 1], HtmlDoc d[static 1]) {
    *ctx = (AhreQjsCtx){.htmldoc=d};
    //size_t initsz = 8;
    //int err = lipfn(ElemPtr,JSValue,init)(&ctx->elements, (LipInitArgs){.sz=initsz});
    //if (err) return "error: lip init failure";
    return Ok;
}


static inline lxb_html_document_t*
ahqctx_lxbdoc(AhreQjsCtx ctx[static 1]) {return htmldoc_lxbdoc(ctx->htmldoc);}


static Err jse_add_document(JsEngine jse[static 1], HtmlDoc d[static 1]);
static JSValue
_document_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

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

    try( jse_add_document(js, htmldoc));
    return Ok;
}


static Err jse_add_document(JsEngine js[static 1], HtmlDoc d[static 1]) {

    JSContext* ctx = js->ctx;

    if (!ctx) return "error: null js ctx";

    ahqctx_init(jse_ahqctx(js), d);
    JS_SetContextOpaque(ctx, jse_ahqctx(js));
    JSValue document = JS_NewObject(ctx);
    JS_SetPropertyStr(
        ctx,
        document,
        "getElementById",
        JS_NewCFunction(ctx, _document_getElementById, "getElementById", 1)
    );
    
    JS_SetPropertyStr(ctx, document, "title", JS_NewString(ctx, "QuickJS Document"));
    JS_SetPropertyStr(ctx, document, "body", JS_NewObject(ctx));
    
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "document", document);
    JS_FreeValue(ctx, global);
    
    return Ok;
}

Err jse_eval(JsEngine js[static 1], Session* s, const char* script) {

    JSContext *ctx = jse_context(js);

    if (!ctx) return "no js context (js engine is enabled with .js)";
    
    JSValue result = JS_Eval(ctx, script, strlen(script), NULL, JS_EVAL_TYPE_GLOBAL);
    
    Err err = Ok;

    if (JS_IsException(result)) {
        JSValue error = JS_GetException(ctx);
        const char *error_str = JS_ToCString(ctx, error);
        err = err_fmt("error evaluting js: %s\n", error_str);
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

Err jse_eval_doc_scripts(Session* s, HtmlDoc d[static 1]) {

    ArlOf(Str)* scripts = htmldoc_scripts(d);
    for (Str* it = arlfn(Str,begin)(scripts); it != arlfn(Str,end)(scripts); ++it) {
       try( jse_eval(htmldoc_js(d), s, items__(it)));
    }
    return Ok;
}

static inline bool _attr_is_valid_(lxb_dom_attr_t* attr) {
    return attr
        && attr->node.owner_document
        ;
}

static inline JSValue _js_value_from_lexbor_element(JSContext *ctx, lxb_dom_element_t* element) {
    JSValue js_element = JS_NewObject(ctx);

    Str* buf = &(Str){0};
    lxb_dom_attr_t* attr = lxb_dom_element_first_attribute(element);

    while (attr) {
        if (!_attr_is_valid_(attr)) return JS_ThrowTypeError(ctx, "internal error atribbute is malformed");
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
            JS_SetPropertyStr(ctx, js_element, items__(buf), JS_NewStringLen(ctx, (char*)value, valuelen));

        attr = lxb_dom_element_next_attribute(attr);
    }
    str_clean(buf);
    JS_SetOpaque(js_element, element);
    return js_element;
}

static JSValue
_document_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val;

    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");

    AhreQjsCtx* ahq_ctx = JS_GetContextOpaque(ctx);

    if (!ahq_ctx) return JS_ThrowTypeError(ctx, "DOM not properly initialized");

    size_t idlen;
    const char *id = JS_ToCStringLen(ctx, &idlen, argv[0]);
    if (!id) return JS_ThrowTypeError(ctx, "Invalid ID");

    lxb_dom_element_t* element = lexbor_doc_get_element_by_id(ahqctx_lxbdoc(ahq_ctx), id, idlen);
    JS_FreeCString(ctx, id);
    if (!element) return JS_NULL;

    return _js_value_from_lexbor_element(ctx, element);
}


//// Create and bind the 'document' object
//JSValue qjs_create_document_object(JSContext *ctx) {
//    JSValue document = JS_NewObject(ctx);
//
//    // Bind methods
//    JS_SetPropertyStr(
//        ctx,
//        document,
//        "getElementById",
//        JS_NewCFunction(ctx, _document_getElementById, "getElementById", 1)
//    );
//
//    // Add properties (optional)
//    JS_SetPropertyStr(ctx, document, "title", JS_NewString(ctx, "Lexbor+QuickJS"));
//    JS_SetPropertyStr(ctx, document, "body", JS_NewObject(ctx));
//
//    return document;
//}

//// Add 'document' to the global scope
//void qjs_add_document_to_global(JSContext *ctx) {
//    JSValue global = JS_GetGlobalObject(ctx);
//    JSValue document = qjs_create_document_object(ctx);
//    JS_SetPropertyStr(ctx, global, "document", document);
//    JS_FreeValue(ctx, global);
//}

//void init_quickjs_with_lexbor(JSContext *ctx, lxb_dom_document_t *lexbor_doc) {
//    // Attach Lexbor document to QuickJS context
//    qjs_init_dom_bindings(ctx, lexbor_doc);
//
//    // Add 'document' to global scope
//    qjs_add_document_to_global(ctx);
//}


////example
//int main() {
//    // Initialize Lexbor
//    lxb_dom_document_t *lexbor_doc = ...; // Your Lexbor document
//
//    // Initialize QuickJS
//    JSRuntime *rt = JS_NewRuntime();
//    JSContext *ctx = JS_NewContext(rt);
//    init_quickjs_with_lexbor(ctx, lexbor_doc);
//
//    // Test JavaScript
//    const char *js_code = "document.getElementById('test').innerHTML = 'Hello!';";
//    JSValue result = JS_Eval(ctx, js_code, strlen(js_code), "<input>", JS_EVAL_TYPE_GLOBAL);
//    if (JS_IsException(result)) {
//        printf("Error executing JS\n");
//    }
//    JS_FreeValue(ctx, result);
//
//    // Cleanup
//    JS_FreeContext(ctx);
//    JS_FreeRuntime(rt);
//    return 0;
//}
