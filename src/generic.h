/*
 * Header file t obe included only on .c files.
 * Here we define the more geenric fucntions
 */
#ifndef __GENERIC_AHRE_H__
#define __GENERIC_AHRE_H__

#if __INCLUDE_LEVEL__ > 1
#error "AHRE: generic.h cannot be included in other header file"
#endif

#include "cmd-params.h"
#include "draw.h"

/* cmd-params.h */
#define get_cmd_out_(P) _Generic((P),\
    CmdParams*: cmd_params_cmd_out,\
    CmdOut*   : cmd_out_id,\
    DrawCtx*  : draw_ctx_cmd_out\
)(P)

#define msg__(P,M)       cmd_out_msg_append(get_cmd_out_(P),M)
#define msg_ln__(P,M)    cmd_out_msg_append_ln(get_cmd_out_(P),M)
#define screen__(P,M)    cmd_out_screen_append(get_cmd_out_(P),M)
#define screen_ln__(P,M) cmd_out_screen_append_ln(get_cmd_out_(P),M)

#define min__(X, Y) _Generic((X),\
    size_t:_Generic((Y), size_t:min_size_t)\
    )((X),(Y))



#define try_or_msg(ErrLval, Label, Expected, Out, Expr)  do{\
    Err ahre_err_=validate_err((Expr)); \
    if (ahre_err_== validate_err(Expected)) msg_ln__(get_cmd_out_(Out), ahre_err_);\
    else try_or_jump(ErrLval, Label, ahre_err_); \
}while(0)


#endif
