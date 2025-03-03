#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#define BT char
#include <buf.h>

typedef const char const_char;
#define BT const_char
#include <buf.h>

#define T char
#include <arl.h>

#define T size_t
#include <arl.h>

typedef const char* const_char_ptr;
#define T const_char_ptr
#include <arl.h>

static inline int buf_of_char_cmp(const void* xp, const void* yp) {
    const BufOf(char)*x = xp;
    const BufOf(char)*y = yp;
    size_t min = (x->len < y->len) ? x->len : y->len;
    return strncmp(x->items, y->items, min);
}

#define T BufOf(char)
#define TClean buffn(char,clean)
#define TCmp buf_of_char_cmp
#include <arl.h>

#include "src/str.h"
#include "src/error.h"


#define buf_append_lit(LitStr, Buf_) do{\
   if (buffn(char,append)(Buf_, (char*)LitStr, sizeof(LitStr)-1)) {\
       return "could not append"; }}while(0)

typedef enum { http_get = 0, http_post = 1 } HttpMethod;

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
write_to_file(const unsigned char* data, size_t len, void* f) {
    return len - fwrite(data, 1, len, f);
}

static inline Err
bufofchar_append_ui_as_str(BufOf(char) buf[static 1], uintmax_t ui) {
    char numbuf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbuf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        //TODO: provide more info about the error
        return "error: could not convert ui to str";
    }
    if (buffn(char, append)(buf, numbuf, len)) return Ok;
    return "error: could not append str_ui to bufof char";
}

Err uint_to_base36_str(char* buf, size_t buf_sz, int n, size_t len[static 1]);


static inline  Err
bufofchar_append_ui_base36_as_str(BufOf(char) buf[static 1], uintmax_t ui) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
    if (!buffn(char, append)(buf, numbf, len)) return "error appending unsigned to bufof char";
    return Ok;
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


static inline  Err
unsigned_to_str(uintmax_t ui, char* buf, size_t size , size_t len[static 1]) {
    if ((*len = snprintf(buf, size, "%lu", ui)) > size) {
        return "error: number too large for string buffer";
    }
    return Ok;
}

static inline  Err
append_unsigned_to_str(uintmax_t ui, char* str, size_t size, size_t len[static 1]) {
    size_t numlen;
    if ((numlen = snprintf(str + *len, size, "%lu", ui)) > (size - *len)) {
        return "error: number too large for string buffer";
    }
    *len += numlen;
    return Ok;
}

static inline 
void str_reverse(char* s, size_t n) {
    if (n < 2) return;
    for (size_t l = 0, r = n-1; l < r; ++l, r--) {
        char tmp = s[l];
        s[l] = s[r];
        s[r] = tmp;
    }
}


Err uint_to_base36_str(char* buf, size_t buf_sz, int n, size_t len[static 1]);
Err parse_base36_or_throw(const char** strptr, unsigned long long* num);
const char* parse_ull(const char* tk, uintmax_t* ullp);

static inline Err bufofchar_append(BufOf(char) buf[static 1], char* s, size_t len) {
    if (buffn(char, append)(buf, s, len)) return Ok;
    buffn(char,clean)(buf);
    return "error: BufOf(char) failed to append.";
}

#define bufofchar_append_lit__(Buffer, LitStr) \
    bufofchar_append(Buffer, LitStr, sizeof(LitStr)-1)

Err parse_size_t_or_throw(const char** strptr, size_t* num, int base) ;

/* * */
#define Str2 BufOf(char)
#define str2_clean buffn(char,clean)
#define str2_append(Str2Ptr, Items, NItems) \
    (buffn(char,append)(Str2Ptr, Items, NItems) ? Ok : "error: str2_append failure")
#endif
