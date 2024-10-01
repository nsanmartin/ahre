#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define BT char
#include <buf.h>

#define T size_t
#include <arl.h>

#include "src/str.h"
#include "src/error.h"


#define LENGTH(Ptr) (Ptr)->len


#define len(Ptr) _Generic((Ptr), \
        Str*: str_len, \
        const Str*: str_len, \
        TextBuf*: textbuf_len, \
        const TextBuf*: textbuf_len \
    )(Ptr)


#define buf_append_lit(LitStr, Buf_) do{\
   if (buffn(char,append)(Buf_, (char*)LitStr, sizeof(LitStr))) {\
       return "could not append"; }}while(0)

static inline int buf_append_hexp(void* p, BufOf(char)*buf) {
    char num_buff[256];
    int len = snprintf(num_buff, 256, "%p", p);
    if (len >= 256) {
        fprintf(stdout, "truncating pointer address!\n");
        return -1;
    }
    if (!buffn(char,append)(buf, num_buff, len)) { return -1; }
    return 0;
}

static inline void log_info(const char* msg) { puts(msg); }
static inline void log_error(const char* msg) { perror(msg); }

#endif
