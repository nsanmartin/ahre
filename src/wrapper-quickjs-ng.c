#include <stdio.h>
#include "quickjs.h"

#include "error.h"
#include "session.h"

Err qjs_eval(Session s[static 1], const char* script) {
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    
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

