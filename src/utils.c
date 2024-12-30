#include <errno.h>
#include "src/utils.h"

const char _base36digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


Err uint_to_base36_str(char* buf, size_t buf_sz, int n, size_t len[static 1]) {
    if (buf_sz == 0) return "error: not enough size to convert num to base36 str.";
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


Err parse_base36_or_throw(const char** strptr, unsigned long long* num) {
    if (!strptr || !*strptr) return "error: unexpected NULL ptr";
    while(**strptr && isspace(**strptr)) ++*strptr;
    if (!**strptr) return "number not given";
    if (!isalnum(**strptr)) return "number must consist in digits";
    char* endptr = NULL;
    *num = strtoull(*strptr, &endptr, 36);
    if (errno == ERANGE)
        return "warn: number out of range";
    if (*strptr == endptr)
        return "warn: number could not be parsed";
    if (*num > SIZE_MAX) 
        return "warn: number out of range (exceeds size max)";
    *strptr = endptr;
    return Ok;
}

