#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define BT char
#include <buf.h>

#define T size_t
#include <arl.h>

#include "src/str.h"
#include "src/error.h"

#define EscCodeRed   "\033[91m"
#define EscCodeBlue  "\033[34m"
#define EscCodeReset "\033[0m"


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

typedef unsigned int (*SerializeCallback)(const unsigned char* data, size_t len, void* ctx);
static inline unsigned int
write_to_file(const unsigned char* data, size_t len, void* f) 
{
    return len - fwrite(data, 1, len, f);
}

static inline unsigned int
serialize_unsigned(SerializeCallback cb, uintmax_t ui, void* ctx, unsigned int error_value) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        return error_value;
    }
    return cb((unsigned char*)numbf, len, ctx);
}

#endif
