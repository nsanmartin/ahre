#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>


typedef const char* Err;
//constexpr Err Ok = (Err)0x0;
#define Ok ((Err)0x0)

Err err_fmt(Err fmt, ...);

#endif

