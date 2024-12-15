#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>


typedef const char* Err;
//constexpr Err Ok = (Err)0x0;
#define Ok ((Err)0x0)

Err err_fmt(Err fmt, ...);


#define validate_err(Value) _Generic((Value), Err: Value)
#define validate_uint(value) _Generic((value), \
        unsigned int: value)
#define validate_size(value) _Generic((value), \
        size_t: value)

// 'try' macro is similar to try keyword in zig. It receives an
// lvalue (a variable) than can store an Err value and an Err
// value. If the Err value is not null (not Ok) then it immediately
// returns that value. If not it does nothing.
#define try(LV, ErrValue) if ((LV=validate_err(ErrValue))) return LV 
#define try_or(ErrValue, RetVal) \
    if (validate_err(ErrValue)) return validate_err(RetVal)

#define try_lxb(Value, RetVal) \
    if ((LXB_STATUS_OK!=validate_uint(Value))) \
        return RetVal

#define try_nonzero(Value, RetVal) \
    if ((!validate_size(Value))) \
        return RetVal

#endif

