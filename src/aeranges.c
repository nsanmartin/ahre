#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include <ah/utils.h>
#include <ah/ctx.h>
#include <ah/buf.h>
#include <ah/ranges.h>

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


static const char* ad_parse_ull(const char* tk, long long unsigned* ullp) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *ullp = strtoll(tk, &endptr, 10);
    if (ERANGE == errno && ULLONG_MAX == *ullp) { return NULL; }
    return endptr == tk ? NULL: endptr;
}

static const char* ae_range_parse_addr_num(const char* tk, unsigned long long num, unsigned long long maximum, size_t* out) {
    unsigned long long ull;
    if (*tk == '+') {
        ++tk;
        const char* rest = ad_parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        else { tk = rest; }
        num += ull; 
        if (num < ull || num > maximum) { return NULL; }
    } else if (*tk == '-') {
        ++tk;
        const char* rest = ad_parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        else { tk = rest; }
        if (ull > num) { return NULL; }
        num -= ull; 
        if (num > maximum) { return NULL; }
    }
    *out = num;
    return tk;
}

_Static_assert(sizeof(size_t) <= sizeof(unsigned long long), "range parser requires this precondition");

static const char* ae_range_parse_addr(const char* tk, unsigned long long curr, unsigned long long max, size_t* out) {
    if (!tk || !*tk) { return NULL; }
    if (*tk == '.' || *tk == '+' || *tk == '-')  {
        if (*tk == '.') { ++tk; }
        const char* rest = ae_range_parse_addr_num(tk, curr, max, out);
        if (rest) { tk = rest; }
        return tk;
    } else if (*tk == '$') {
        ++tk;
        const char* rest = ae_range_parse_addr_num(tk, max, max, out);
        if (rest) { tk = rest; }
        return tk;
    } else if (isdigit(*tk)) {
        /* tk does not start neither with - nor + */
        const char* rest = ad_parse_ull(tk, &curr);
        if (!rest /* overflow  || curr > max*/) { return NULL; }
        *out = curr;
        if (*tk == '+' || *tk == '-') {
            const char* rest = ae_range_parse_addr_num(tk+1, curr, max, out);
            if (rest) { return rest; }
        }
        return rest;
    } 

    if (*tk == '+' || *tk == '-') {
        const char* rest = ae_range_parse_addr_num(tk+1, curr, max, out);
        if (rest) { return rest; }
    }
    return NULL;
}


const char* ad_range_parse_impl(
    const char* tk, size_t current_line, size_t nlines, AeRange* range
) {
    range->no_range = false;

    /* invalid input */
    if (!tk) { return NULL; }
    tk = skip_space(tk);

    /* empty string */
    if (!*tk) { 
        puts("TODO: this does not happend 0");
        range->end = range->beg = current_line;
        range->no_range = true;
        return tk;
    }

    /* full range */
    if (*tk == '%') {
        ++tk;
        *range = (AeRange){.beg=1, .end=nlines};
        return tk;
    } 

    /* ,Addr? */
    if (*tk == ',') {
        ++tk;
        range->beg = current_line;
        const char* rest = ae_range_parse_addr(tk, current_line, nlines, &range->end);
        return rest? rest : tk;
    }
            

    const char* rest = ae_range_parse_addr(tk, current_line, nlines, &range->beg);
    if (rest) { 
        /* Addr... */
        tk = skip_space(rest);
        if (*tk == ',') {
            ++tk;
            /* Addr,... */
            rest = ae_range_parse_addr(tk, current_line, nlines, &range->end);
            if (rest) {
                /* Addr,AddrEOF */
                return rest;
            } else {
                /* Addr,EOF */
                range->end = nlines;
                return tk;
            }
        } else {
            /* AddrEOF */
            range->end = range->beg;
            return tk;
        }
    } else {
        /* emptyEOF */
        *range = (AeRange){.beg=current_line, .end=current_line};
        range->no_range = true;
        return tk;
    }

}

inline const char*
ad_range_parse(const char* tk, AhCtx ctx[static 1], AeRange* range) {
    AeBuf* aeb = AhCtxCurrentBuf(ctx);
    size_t current_line = aeb->current_line;
    size_t nlines       = AeBufNLines(aeb);
    return ad_range_parse_impl(tk, current_line, nlines, range);
}
