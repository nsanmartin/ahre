#include <stdio.h>

#include "error.h"
#include "session.h"
#include "js-engine.h"

static JSValue
_qjs_document_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc != 1) return JS_ThrowTypeError(ctx, "Expected 1 argument");
    
    const char *id_name = JS_ToCString(ctx, argv[0]);
    printf("document.getElementById('%s') called\n", id_name);
    
    JSValue element = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, element, "id", JS_NewString(ctx, id_name));
    JS_SetPropertyStr(ctx, element, "innerHTML", JS_NewString(ctx, ""));
    
    JS_FreeCString(ctx, id_name);
    return element;
}

Err qjs_add_document(JSContext* ctx) {
    if (!ctx) return "error: null js ctx";
    JSValue document = JS_NewObject(ctx);
    
    JS_SetPropertyStr(
        ctx,
        document,
        "getElementById",
        JS_NewCFunction(ctx, _qjs_document_getElementById, "getElementById", 1)
    );
    
    JS_SetPropertyStr(ctx, document, "title", JS_NewString(ctx, "QuickJS Document"));
    JS_SetPropertyStr(ctx, document, "body", JS_NewObject(ctx));
    
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "document", document);
    JS_FreeValue(ctx, global);
    
    return Ok;
}

//TODO: delete
Err qjs_eval(Session s[static 1], const char* script) {
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    
    try( qjs_add_document(ctx));
//
//  JS_Eval(
//      JSContext *ctx,
//      const char *input,
//      size_t input_len,
//      const char *filename,
//      int eval_flags
//  )
//
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
    
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return err;
}



Err jse_add_document(JsEngine js[static 1]) {
    JSContext* ctx = js->ctx;
    if (!ctx) return "error: null js ctx";
    JSValue document = JS_NewObject(ctx);
    
    JS_SetPropertyStr(
        ctx,
        document,
        "getElementById",
        JS_NewCFunction(ctx, _qjs_document_getElementById, "getElementById", 1)
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
