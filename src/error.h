#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include "mem.h"

#include <stdio.h>
#include <stdbool.h>

#define MAX_MSG_LEN 4090u

typedef const char* Err;
#define Ok ((Err)0x0)

static inline Err err_skip(void) { return Ok; }

#define INTERNAL_ERROR_PREFIX "error"

static inline bool internal_error(Err e) {
    return !strncmp(e, INTERNAL_ERROR_PREFIX, sizeof(INTERNAL_ERROR_PREFIX) - 1);
}

#define file_line__  __FILE__ ":" to_lit__(__LINE__)


bool simulate_error(void);

#define validate_err__(Value) _Generic((Value), Err: Value)

#ifdef AHRE_SIMULATE_ERR
#define validate_err(Value) (simulate_error()? "TEST: SIMULATED ERROR " file_line__: validate_err__(Value))
#else
#define validate_err validate_err__
#endif

#define validate_int(Value) _Generic((Value), int: Value)
#define validate_uint(value) _Generic((value), \
        unsigned int: value)
#define validate_size(value) _Generic((value), \
        size_t: value)

Err _err_fmt_vsnprinf_(Err fmt, ...);

#define err_fmt(Fmt, ...) _err_fmt_vsnprinf_(Fmt,__VA_ARGS__)
#define to_lit__impl__(X) #X
#define to_lit__(X) to_lit__impl__(X)

// 'try' macro is similar to try keyword in zig. It receives an expression of
// type Err value. If the Err value is not null (not Ok) then it immediately
// returns that value. If not it does nothing.
#define try(Expr) do{\
    Err ahre_err_=validate_err((Expr));if (ahre_err_) return ahre_err_;}while(0) 

#define tryjmp(ErrLval, Label, Expr) do{\
    ErrLval=validate_err((Expr));if (ErrLval) goto Label;}while(0)

#define ok_then(Error, Expr) do{ if (!Error) { Error=validate_err((Expr));}} while(0) 

#define tryz(Expr) do{int ahre_ierr_=validate_int((Expr));if (ahre_ierr_) return ahre_ierr_;}while(0) 

#define REAL_ERR_MSG_ "a real error: "

// Err err_prepend_char(Err err, char c);


// #define err_jump(E,Tag) do{E="error: " __FILE__ ":" to_lit__(__LINE__); goto Tag;}while(0)

#define err_internal(Msg) err_fmt("error: %s  %s\n", Msg, file_line__)
#define err_append() err_internal("append failure")

#endif
