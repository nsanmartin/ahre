#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include <ahutils.h>
#include <ahctx.h>
#include <ae-buf.h>
#include <ae-ranges.h>

/*
 * + . current line,
 * + $ last line in the buffer
 * + 1 first line in the buffer
 * + 0 line before the first line
 * + 't is the line in which mark t is placed
 * + '< and '>  the previous selection begins and ends
 * + /pat/ is the next line where pat matches
 * + \/ the next line in which the previous search pattern matches
 * + \& the next line in which the previous substitute pattern matches 
*/

//char* ad_parse_ll(char* tk, long long* llp) {
//    char* endptr = 0x0;
//    long long ll = strtoll(tk, &endptr, 10);
//    if (ERANGE == errno && (LONG_MAX == ll || LONG_MIN == ll)) { return NULL; }
//    *llp = ll;
//    return endptr;
//}

char* ad_parse_ull(char* tk, long long unsigned* ullp) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *ullp = strtoll(tk, &endptr, 10);
    if (ERANGE == errno && ULLONG_MAX == *ullp) { return NULL; }
    return endptr;
}

char* ad_range_parse_addr_num(char* tk, unsigned long long num, unsigned long long maximum, size_t* out) {
    unsigned long long ull;
    if (*tk == '+') {
        ++tk;
        char* rest = ad_parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        num += ull; 
        if (num < ull || num > maximum) { return NULL; }
        tk = rest;
    } else if (*tk == '-') {
        ++tk;
        char* rest = ad_parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        if (ull > num) { return NULL; }
        num -= ull; 
        if (num > maximum) { return NULL; }
        tk = rest;
    }
    *out = num;
    return tk;
}

char* ad_range_parse_addr(char* tk, AhCtx ctx[static 1], size_t* out) {
    AeBuf* aeb = ahctx_current_buf(ctx);
    if (!tk || !*tk) { return NULL; }
    unsigned long long linenum = aeb->current_line;
    unsigned long long maximum = aeb->buf.len ? aeb->buf.len - 1 : 0;
    if (*tk == '.')  {
        ++tk;
        char* rest = ad_range_parse_addr_num(tk, linenum, maximum, out);
        if (rest) { tk = rest; }
        return tk;
    }
    if (*tk == '$') {
        ++tk;
        char* rest = ad_range_parse_addr_num(tk, maximum, maximum, out);
        if (rest) { tk = rest; }
        return tk;
    }
    if (isdigit(*tk)) {
        // tk does not start neither with - nor +
        char* rest = ad_parse_ull(tk, &linenum);
        if (!rest /* overflow */ || linenum > maximum) { return NULL; }
        *out = linenum;
        return rest;
    }
    return NULL;
}

//int ad_range_parse_end(char* tk, size_t* end) {
//}
//

char* ad_range_parse(char* tk, AhCtx ctx[static 1], AeRange* range) {
    if (!tk || !*tk) { return NULL; }
    if (*tk == '%') {
        ++tk;
        size_t len = ahctx_current_buf(ctx)->buf.len;
        if (!len) { return NULL; }
        *range = (AeRange){.beg=0, .end=len-1};
        return tk;
    }

    char* rest = ad_range_parse_addr(tk, ctx, &range->beg);
    if (!rest) { return NULL; }
    rest = skip_space(rest);
    tk = rest;
    if (*tk == ',')  {
        ++tk;
        rest = ad_range_parse_addr(tk, ctx, &range->end);
        if (!rest) { range->end = ahctx_current_buf(ctx)->current_line; }
        tk = rest;
    } else {
        range->end = range->beg;
    }
    return rest;
}
