#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>


typedef const char* Err;
//constexpr Err Ok = (Err)0x0;
#define Ok ((Err)0x0)

static inline Err err_skip(void) { return Ok; }
Err err_fmt(Err fmt, ...);

static Err FATAL_ERROR_PREFIX = "error";
static size_t FATAL_ERROR_PREFIX_LEN = sizeof(FATAL_ERROR_PREFIX) - 1;

static inline bool fatal_error(Err e) {
    return !strncmp(e, FATAL_ERROR_PREFIX, FATAL_ERROR_PREFIX_LEN);
}

#define validate_err(Value) _Generic((Value), Err: Value)
#define validate_uint(value) _Generic((value), \
        unsigned int: value)
#define validate_size(value) _Generic((value), \
        size_t: value)

// 'try' macro is similar to try keyword in zig. It receives an expression of
// type Err value. If the Err value is not null (not Ok) then it immediately
// returns that value. If not it does nothing.
#define try(Expr) do{Err ahre_err_=validate_err((Expr));if (ahre_err_) return ahre_err_;}while(0) 

#define ok_then(Error, Expr) do{ if (!Error) { Error=validate_err((Expr));}} while(0) 

#define trygotoerr(LV, Tag, ErrValue) if ((ErrValue)) do{ LV=ErrValue; goto Tag;}while(0)

#define try_lxb(Value, RetVal) \
    if ((LXB_STATUS_OK!=validate_uint(Value))) \
        return RetVal

#define try_nonzero(Value, RetVal) \
    if ((!validate_size(Value))) \
        return RetVal

#define REAL_ERR_MSG_ "a real error: "

Err err_prepend_char(Err err, char c);
#endif
