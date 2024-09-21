#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>

typedef const char* ErrStr;
typedef enum { Ok = 0, ErrMem, ErrLxb, ErrCurl, ErrFile } Error;

#define RETERR(ERRMSG,RETVAL) do{ perror(ERRMSG); return RETVAL; } while(0)

#endif

