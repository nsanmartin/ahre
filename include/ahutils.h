#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

#include <aherror.h>

#define BT char
#include <buf.h>

#define buf_append_lit(LitStr, Buf_) do{\
   if (buffn(char,append)(Buf_, (char*)LitStr, sizeof(LitStr))) {\
       return -1; }}while(0)

static inline int buf_append_hexp(void* p, BufOf(char)*buf) {
    char num_buff[256]; //TODO: what if pointer may be bigger than 10^256?
    int len = snprintf(num_buff, 256, "%p", p);
    if (len) {
       if (buffn(char,append)(buf, num_buff, len)) {
           return -1;
       }
    }
    return 0;
}

typedef struct {
	const char* s;
	size_t len;
} Str;

static inline void ah_log_info(const char* msg) { puts(msg); }
static inline Error ah_log_error(const char* msg, Error e) {
    perror(msg); return e;
}


#endif
