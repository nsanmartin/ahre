#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include <ah/utils.h>
#include <ah/session.h>
#include <ah/textbuf.h>
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


/* internal linkage */

static const char* parse_ull(const char* tk, long long unsigned* ullp) {
    if (!tk || !*tk) { return NULL; }
    char* endptr = 0x0;
    *ullp = strtoll(tk, &endptr, 10);
    if (ERANGE == errno && ULLONG_MAX == *ullp) { return NULL; }
    return endptr == tk ? NULL: endptr;
}

static const char* range_parse_addr_num(const char* tk, unsigned long long num, unsigned long long maximum, size_t* out) {
    unsigned long long ull;
    if (*tk == '+') {
        ++tk;
        const char* rest = parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        else { tk = rest; }
        num += ull; 
        if (num < ull || num > maximum) { return NULL; }
    } else if (*tk == '-') {
        ++tk;
        const char* rest = parse_ull(tk, &ull);
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

static const char* range_parse_addr(const char* tk, unsigned long long curr, unsigned long long max, size_t* out) {
    if (!tk || !*tk) { return NULL; }
    if (*tk == '.' || *tk == '+' || *tk == '-')  {
        if (*tk == '.') { ++tk; }
        const char* rest = range_parse_addr_num(tk, curr, max, out);
        if (rest) { tk = rest; }
        return tk;
    } else if (*tk == '$') {
        ++tk;
        const char* rest = range_parse_addr_num(tk, max, max, out);
        if (rest) { tk = rest; }
        return tk;
    } else if (isdigit(*tk)) {
        /* tk does not start neither with - nor + */
        const char* rest = parse_ull(tk, &curr);
        if (!rest /* overflow  || curr > max*/) { return NULL; }
        *out = curr;
        if (*tk == '+' || *tk == '-') {
            const char* rest = range_parse_addr_num(tk+1, curr, max, out);
            if (rest) { return rest; }
        }
        return rest;
    } 

    if (*tk == '+' || *tk == '-') {
        const char* rest = range_parse_addr_num(tk+1, curr, max, out);
        if (rest) { return rest; }
    }
    return NULL;
}


static const char* range_parse_impl(const char* tk, size_t current_line, size_t nlines, Range* range) {
    /* invalid input */
    if (!tk) { return NULL; }
    tk = skip_space(tk);

    /* empty string */
    if (!*tk) { 
        range->beg = current_line;
        range->end = 0;
        return tk;
    }

    /* full range */
    if (*tk == '%') {
        ++tk;
        *range = (Range){.beg=1, .end=nlines};
        return tk;
    } 

    /* ,Addr? */
    if (*tk == ',') {
        ++tk;
        range->beg = current_line;
        const char* rest = range_parse_addr(tk, current_line, nlines, &range->end);
        return rest? rest : tk;
    }
            
    const char* rest = range_parse_addr(tk, current_line, nlines, &range->beg);
    if (rest) { 
        /* Addr... */
        tk = skip_space(rest);
        if (*tk == ',') {
            ++tk;
            /* Addr,... */
            rest = range_parse_addr(tk, current_line, nlines, &range->end);
            if (rest) {
                /* Addr,AddrEOF */
                return rest;
            } else {
                /* Addr,EOF */
                range->end = 0;
                return tk;
            }
        } else {
            /* AddrEOF */
            range->end = range->beg;
            return tk;
        }
    } else {
        /* emptyEOF */
        *range = (Range){.beg=current_line, .end=0};
        return tk;
    }

}

static inline bool is_range_valid(Range r[static 1], size_t nlines) {
    return r->beg <= r->end && r->end <= nlines;
}

static inline bool range_has_no_end(Range r[static 1]) { return r->end == 0; }

/* external linkage */

inline const char*
range_parse(const char* tk, Session session[static 1], Range* range) {
    TextBuf* aeb = session_current_buf(session);
    size_t current_line = aeb->current_line;
    size_t nlines       = textbuf_line_count(aeb);
    tk =  range_parse_impl(tk, current_line, nlines, range);
    if (!tk || (!range_has_no_end(range) && is_range_valid(range, 0))) { return NULL; }
    else if (range->end == 0) range->end = range->beg; 
    return tk;
}

