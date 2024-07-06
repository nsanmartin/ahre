#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <stddef.h>
#include <stdio.h>

#include <aherror.h>

typedef struct {
	const char* s;
	size_t len;
} Str;

static inline void ah_log_info(const char* msg) { puts(msg); }
static inline Error ah_log_error(const char* msg, Error e) { perror(msg); return e; }


#endif
