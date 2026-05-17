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

#define isnull(StructPtr) (((StructPtr).ptr) == NULL)
Err         parse_ll_err(const char* tk, intmax_t llp[_1_], const char* endptr[_1_]);
Err         parse_size_t_err(const char* tk, size_t out[_1_], const char* endptr[_1_], int base);
Err         parse_size_t_or_throw(const char** strptr, size_t* num, int base) ;
Err         uint_to_base36_str(char* buf, size_t buf_sz, uintmax_t n, size_t len[_1_]);
const char* parse_l(const char* tk, long lptr[_1_]);
const char* parse_ull(const char* tk, uintmax_t* ullp);

static inline bool   file_exists(const char* filename) { return !access(filename, F_OK); }
static inline size_t size_t_min(size_t x, size_t y) { return x <= y ? x : y; }
static inline size_t size_t_max(size_t x, size_t y) { return x >= y ? x : y; }


#define cast__(T) (T)
#define typeof_max__(X) _Generic((X),\
    int     : INT_MAX,\
    size_t  : SIZE_MAX,\
    unsigned: UINT_MAX\
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

#define abs_int__(I) (1+(int)(1+I))


static inline Err _set_size_t_from_int_(size_t s[_1_], int i) {
    if (i < 0) return "error: size_t underflow (cannot be nagative)";
    if (lt__(SIZE_MAX, i)) return "error: size_ overflow";
    *s = cast__(size_t)i;
    return Ok;
}

#define set__(A,B) \
    _Generic((A),\
            size_t*:_Generic((B),\
                int: _set_size_t_from_int_ \
                )\
            )(A,B)


#define skip__(X)
#define GET_MACRO__(_1,_2,_3,NAME,...) NAME

static inline void set_flag(unsigned flags[_1_], unsigned mask, bool value) {
    if (value) *flags |= mask;
    else *flags &= ~mask;
}

#define lit_len__(Lit) (Lit == NULL ? 0 : sizeof(Lit)-1)

typedef const char const_char;
#define T const_char
#include <buf.h>

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


#include "str.h"
#include "error.h"


typedef enum { http_get = 0, http_post = 1 } HttpMethod;


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


/* for Type, Collection, Element ... */
#define foreach__(T,Col,It) \
    for (T* It = arlfn(T,begin)(Col) ; It < arlfn(T,end)(Col) ; ++It)


#endif
