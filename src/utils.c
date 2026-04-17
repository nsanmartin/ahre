#include <errno.h>

#include "utils.h"


const char _base36digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


const char* parse_ull(const char* tk, uintmax_t* ullp) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *ullp = strtoull(tk, &endptr, 10);
    if (ERANGE == errno && ULLONG_MAX == *ullp) { return NULL; }
    return endptr == tk ? NULL: endptr;
}


const char* parse_l(const char* tk, long lptr[_1_]) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *lptr = strtol(tk, &endptr, 10);
    if (ERANGE == errno && (LONG_MAX == *lptr || LONG_MIN == *lptr)) { return NULL; }
    return endptr == tk ? NULL: endptr;
}

Err uint_to_base36_str(char* buf, size_t buf_sz, uintmax_t n, size_t len[_1_]) {
    if (buf_sz == 0) return "error: not enough size to convert num to base36 str.";
    *len = 0;
    do {
        *buf++ = _base36digits[n % 36];
        n /= 36;
        --buf_sz;
        ++*len;
    } while (n != 0 && buf_sz);

    if (!buf_sz && n) return "error: not enough size to convert num to base36 str.";
    str_reverse(buf - *len, *len);

    return Ok;
}

Err parse_size_t_or_throw(const char** strptr, size_t* num, int base) {
    if (!strptr || !*strptr) return "error: unexpected NULL ptr";
    const char* str = cstr_skip_space(*strptr);
    if (!*str) return "number not given";
    if (!isalnum(*str)) return "number must consist in alnum chars";
    char* endptr = NULL;
    *num = strtoull(str, &endptr, base);
    if (errno == ERANGE)
        return "warn: number out of range";
    if (str== endptr)
        return "warn: number could not be parsed";
    if (*num > SIZE_MAX) 
        return "warn: number out of range (exceeds size max)";
    *strptr = endptr;
    return Ok;
}


Err parse_ll_err(const char* tk, intmax_t llp[_1_], const char* endptr[_1_]) {
    if (!tk) return "error parsing ll: NULL received";
    if (!*tk) return "error parsing ll: \"\\0\" received";
    *llp = strtoll(tk, (char**)endptr, 10);
    if (ERANGE == errno && (LLONG_MAX == *llp || LLONG_MIN == *llp))
       return err_fmt("error: %s", strerror(errno));
    return Ok;
}


Err parse_size_t_err(const char* tk, size_t out[_1_], const char* endptr[_1_], int base) {
    if (!tk) return "error parsing size_t: NULL received";
    if (!*tk) return "error parsing size_t: \"\\0\" received";
    long long unsigned llu = strtoull(tk, (char**)endptr, base);
    if (ERANGE == errno) return err_fmt("error: %s", strerror(errno));
    if (llu > SIZE_MAX) return "error: size_t too small for num";
    *out = (size_t)llu;
    return Ok;
}
