#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>

#include "mem.h"
#include "error.h"

#define UINT_TO_STR_BUFSZ 64
#define INT_TO_STR_BUFSZ    64

static inline size_t size_t_min(size_t x, size_t y) { return x > y ? x : y; }
inline static bool file_exists(const char* filename) { return !access(filename, F_OK); }

#define skip__(X)
#define GET_MACRO__(_1,_2,_3,NAME,...) NAME

#define lit_len__(Lit) (Lit == NULL ? 0 : sizeof(Lit)-1)
#define T char
#include <buf.h>

typedef const char const_char;
#define T const_char
#include <buf.h>

#define T char
#include <arl.h>

#define T size_t
#include <arl.h>

typedef const char* cstr_view;
#define T cstr_view
#include <arl.h>

typedef const char* const_cstr;
static inline void const_cstr_free(const_cstr* p) { std_free((void*)*p); }
#define T const_cstr
#define TClean const_cstr_free
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

#include "str.h"
#include "error.h"


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


Err uint_to_base36_str(char* buf, size_t buf_sz, int n, size_t len[static 1]);


static inline  Err
bufofchar_append_ui_base36_as_str(BufOf(char) buf[static 1], uintmax_t ui) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
    if (!buffn(char, append)(buf, numbf, len)) return "error appending unsigned to bufof char";
    return Ok;
}


static inline  Err
unsigned_to_str(uintmax_t ui, char* buf, size_t size , size_t len[static 1]) {
    if ((*len = snprintf(buf, size, "%lu", ui)) > size) {
        return "error: number too large for string buffer";
    }
    return Ok;
}

static inline  Err
int_to_str(intmax_t i, char* buf, size_t size , size_t len[static 1]) {
    if ((*len = snprintf(buf, size, "%ld", i)) > size) {
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
const char* parse_ull(const char* tk, uintmax_t* ullp);
Err parse_ull_err(const char* tk, uintmax_t ullp[static 1], const char* endptr[static 1]);
Err parse_ll_err(const char* tk, intmax_t llp[static 1], const char* endptr[static 1]);

static inline Err bufofchar_append(BufOf(char) buf[static 1], char* s, size_t len) {
    if (buffn(char, append)(buf, s, len)) return Ok;
    buffn(char,clean)(buf);
    return "error: BufOf(char) failed to append.";
}

#define bufofchar_append_lit__(Buffer, LitStr) \
    bufofchar_append(Buffer, LitStr, sizeof(LitStr)-1)

Err parse_size_t_or_throw(const char** strptr, size_t* num, int base) ;


#define foreach__(T,It,Col) \
    for (T* It = arlfn(T,begin)(Col) ; It < arlfn(T,end)(Col) ; ++It)


#define file_write_or_close(Mem, Len, Fp) _file_write_or_close_(Mem, Len, Fp, __FILE__)
#define file_write(Mem, Len, Fp) _file_write_(Mem, Len, Fp, __FILE__)
#define file_write_lit_sep(Mem, Len, LitSep, Fp) \
    _file_write_sep_(Mem, Len, LitSep, lit_len__(LitSep), Fp, __FILE__)

#define file_write_int_lit_sep(Int, LitSep, Fp) \
    _file_write_int_sep_(Int, LitSep, lit_len__(LitSep), Fp, __FILE__)

Err _file_write_int_sep_(intmax_t i, const char* sep, size_t seplen, FILE* fp, const char* caller);

Err _file_write_or_close_(const char* mem, size_t len, FILE* fp, const char* caller);
Err file_open(const char* filename, const char* mode, FILE* fpp[static 1]);
Err _file_write_(const char* mem, size_t len, FILE* fp, const char* caller);
Err file_close(FILE* fp);
Err _file_write_sep_(
    const char* mem, size_t len, const char* sep, size_t seplen, FILE* fp, const char* caller
);
#endif
