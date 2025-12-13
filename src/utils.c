#include <errno.h>

#include "utils.h"


/* file utils */

Err file_open(const char* filename, const char* mode, FILE* fpp[_1_]) {
    *fpp = fopen(filename, mode);
    if (!*fpp)
        return err_fmt("error opening file '%s': %s", filename, strerror(errno));
    return Ok;
}

Err _file_write_(const char* mem, size_t len, FILE* fp, const char* caller) {
    if (len && fwrite(mem, 1, len, fp) != len)
        return err_fmt("(%s) error writing to file: %s", caller, strerror(errno));
    return Ok;
} 

Err _file_write_sep_(
    const char* mem, size_t len, const char* sep, size_t seplen, FILE* fp, const char* caller
) {
    if (len && fwrite(mem, 1, len, fp) != len)
        return err_fmt("(%s) error writing to file: %s", caller, strerror(errno));
    if (seplen && fwrite(sep, 1, seplen, fp) != seplen)
        return err_fmt("(%s) error writing to file: %s", caller, strerror(errno));
    return Ok;
} 


Err _file_write_int_sep_(intmax_t i, const char* sep, size_t seplen, FILE* fp, const char* caller) {
    char buf [INT_TO_STR_BUFSZ];
    size_t len;
    try( int_to_str(i, buf, sizeof buf, &len));
    return _file_write_sep_(buf, len, sep, seplen, fp, caller);
}
                

Err _file_write_or_close_(const char* mem, size_t len, FILE* fp, const char* caller) {
    if (len && fwrite(mem, 1, len, fp) != len) {
        const char* fwrite_strerr = strerror(errno);
        bool close_err = fclose(fp);
        if (close_err) {
            const char* fclose_strerr = strerror(errno);
            return err_fmt(
                "error: both fwrite (%s) and fclose (%s) failed (%s)",
                fwrite_strerr, fclose_strerr, caller
            );
        }
        return err_fmt("error: fwrite failed: %s (%s)", fwrite_strerr, caller);
    }
    return Ok;
} 

Err file_close(FILE* fp) {
    if (fclose(fp)) return err_fmt("error closing file: %s", strerror(errno));
    return Ok;
}

// file utils

const char _base36digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/* note that if something like "   -1" is received, 2^NBits -1 is returned */
Err parse_ull_err(const char* tk, uintmax_t ullp[_1_], const char* endptr[_1_]) {
    if (!tk) return "error parsing ull: NULL received";
    if (!*tk) return "error parsing ull: \"\\0\" received";
    *ullp = strtoull(tk, (char**)endptr, 10);
    if (ERANGE == errno && ULLONG_MAX == *ullp) { return err_fmt("error: %s", strerror(errno)); }
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

const char* parse_ull(const char* tk, uintmax_t* ullp) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *ullp = strtoull(tk, &endptr, 10);
    if (ERANGE == errno && ULLONG_MAX == *ullp) { return NULL; }
    return endptr == tk ? NULL: endptr;
}


const char* parse_ll(const char* tk, intmax_t* llp) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *llp = strtoll(tk, &endptr, 10);
    if (ERANGE == errno && (LLONG_MAX == *llp||LLONG_MIN == *llp)) { return NULL; }
    return endptr == tk ? NULL: endptr;
}

const char* parse_l(const char* tk, long lptr[_1_]) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *lptr = strtol(tk, &endptr, 10);
    if (ERANGE == errno && (LONG_MAX == *lptr || LONG_MIN == *lptr)) { return NULL; }
    return endptr == tk ? NULL: endptr;
}

Err uint_to_base36_str(char* buf, size_t buf_sz, int n, size_t len[_1_]) {
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
    while(*str && isspace(*str)) ++str;
    if (!*str) return "number not given";
    if (!isalnum(*str)) return "number must consist in digits";
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


