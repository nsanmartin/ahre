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

#define _1_ 1
#define UINT_TO_STR_BUFSZ 64
#define INT_TO_STR_BUFSZ    64


Err parse_ll_err(const char* tk, intmax_t llp[_1_], const char* endptr[_1_]);
Err parse_size_t_err(const char* tk, size_t out[_1_], const char* endptr[_1_], int base);
Err parse_size_t_or_throw(const char** strptr, size_t* num, int base) ;
Err uint_to_base36_str(char* buf, size_t buf_sz, uintmax_t n, size_t len[_1_]);
const char* parse_ull(const char* tk, uintmax_t* ullp);
inline static bool file_exists(const char* filename) { return !access(filename, F_OK); }
static inline size_t size_t_min(size_t x, size_t y) { return x > y ? x : y; }


#define cast__(T) (T)

#define typeof_max__(X) _Generic((X),\
    int   :INT_MAX,\
    size_t:SIZE_MAX \
)

#define _less_than_(AType, A, BType, B) \
    (  (typeof_max__(A) >= typeof_max__(B) && (A < (AType)B)) \
    || (typeof_max__(A) <  typeof_max__(B) && ((BType)A < (AType)B)) )

static inline bool _size_t_lt_int_(size_t s, int i) {
    return i > 0 && _less_than_(size_t, s, int, i);
}

#define lt__(A,B) \
    _Generic((A),\
            size_t:_Generic((B),\
                int: _size_t_lt_int_ \
                )\
            )(A,B)


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
    if (!buffn(char,append)(buf, num_buff, (size_t)len)) { return -1; }
    return 0;
}


static inline  Err unsigned_to_str(uintmax_t ui, char* buf, size_t size , size_t len[_1_]) {
    int printlen = snprintf(buf, size, "%lu", ui);
    if (printlen < 0) return "error: snprintf returned 0";
    if (lt__(size, printlen)) return "error: number too large for string buffer";
    *len = cast__(size_t)printlen;
    return Ok;
}


static inline  Err int_to_str(intmax_t i, char* buf, size_t size , size_t len[_1_]) {
    int printlen = snprintf(buf, size, "%ld", i);
    if (printlen < 0) return "error: snprintf returned 0";
    if (lt__(size, printlen)) return "error: number too large for string buffer";
    *len = cast__(size_t)printlen;
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
Err file_open(const char* filename, const char* mode, FILE* fpp[_1_]);
Err _file_write_(const char* mem, size_t len, FILE* fp, const char* caller);
Err file_close(FILE* fp);
Err _file_write_sep_(
    const char* mem, size_t len, const char* sep, size_t seplen, FILE* fp, const char* caller
);
#endif
