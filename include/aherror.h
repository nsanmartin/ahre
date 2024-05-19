#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>

typedef enum { Ok = 0, ErrMem, ErrLxb, ErrCurl } Error;

static inline Error ah_logging_error(const char* msg, Error e) { perror(msg); return e; }
#endif

