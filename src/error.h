#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>


typedef const char* Err;
//constexpr Err Ok = (Err)0x0;
#define Ok ((Err)0x0)

Err err_fmt(Err fmt, ...);


#define validate_err(Value) _Generic((Value), Err: Value)
#define validate_uint(Value) _Generic((Value), \
        unsigned int: Value)

#define try(LV, ErrValue) if ((LV=validate_err(ErrValue))) return LV 
#define try_or(ErrValue, RetVal) \
    if (validate_err(ErrValue)) return validate_err(RetVal)

#define try_lxb(Value, RetVal) \
    if ((LXB_STATUS_OK!=validate_uint(Value))) \
        return RetVal

#endif

