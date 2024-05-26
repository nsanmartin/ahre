#ifndef __STATUS_AHRE_H__
#define __STATUS_AHRE_H__

#include <stdio.h>

typedef const char* ErrStr;
typedef enum { Ok = 0, ErrMem, ErrLxb, ErrCurl, ErrFile } Error;

static inline void ah_log_err(const char* msg) { fprintf(stderr, "%s\n", msg); }
static inline Error ah_logging_error(const char* msg, Error e) { perror(msg); return e; }
#endif

