#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "src/utils.h"
#include "src/textbuf.h"
#include "src/ranges.h"
#include "src/re.h"

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

static const char* parse_range_addr_num(const char* tk, unsigned long long num, unsigned long long maximum, size_t* out) {
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

static const char* parse_linenum_search_regex(
    const char* tk, TextBuf tb[static 1], size_t out[static 1]
) {
    const char* buf = textbuf_current_line_offset(tb);
    if (!buf) { perror("error: invalid current line"); exit(-1); }
    char* end = strchr(tk, '/');
    const char* res = end ? end + 1 : tk + strlen(tk);
    if (end) *end = '\0';
    size_t match = 0;
    size_t* pmatch = &match;
    Err err = regex_maybe_find_next(tk, buf, &pmatch);
    if (end) *end = '/';

    if (!pmatch) 
        return NULL;
    

    size_t linenum;
    if ((err = textbuf_get_line_of(tb, buf + match, &linenum))) {
        perror(err); exit(-1);
    }

    *out = linenum;
    ///TODO: refactor in order to mutate only in one place
    *textbuf_current_line(tb) = linenum;

    return res;
}

static const char*
parse_range_addr(const char* tk, TextBuf tb[static 1], size_t out[static 1]) {

    unsigned long long curr = *textbuf_current_line(tb);
    unsigned long long max  = textbuf_eol_count(tb);
    if (!tk || !*tk) { return NULL; }
    if (*tk == '/') {
        ++tk;
        return parse_linenum_search_regex(tk, tb, out);
    } else if (*tk == '.' || *tk == '+' || *tk == '-')  {
        if (*tk == '.') { ++tk; }
        const char* rest = parse_range_addr_num(tk, curr, max, out);
        if (rest) { tk = rest; }
        return tk;
    } else if (*tk == '$') {
        ++tk;
        const char* rest = parse_range_addr_num(tk, max, max, out);
        if (rest) { tk = rest; }
        return tk;
    } else if (isdigit(*tk)) {
        /* tk does not start neither with - nor + */
        const char* rest = parse_ull(tk, &curr);
        if (!rest /* overflow  || curr > max*/) { return NULL; }
        *out = curr;
        if (*tk == '+' || *tk == '-') {
            const char* rest = parse_range_addr_num(tk+1, curr, max, out);
            if (rest) { return rest; }
        }
        return rest;
    } 

    if (*tk == '+' || *tk == '-') {
        const char* rest = parse_range_addr_num(tk+1, curr, max, out);
        if (rest) { return rest; }
    }
    return NULL;
}


static const char* parse_range_impl(
    const char* tk, TextBuf tb[static 1], Range range[static 1]
) {
    size_t current_line = *textbuf_current_line(tb);
    size_t nlines       = textbuf_eol_count(tb);


    /* invalid input */
    if (!tk) return NULL;

    tk = cstr_skip_space(tk);

    /* empty string */
    if (!*tk) return NULL;

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
        //size_t* endp = &range->end;
        const char* rest = parse_range_addr(tk, tb, &range->end);
        //if (rest && !*endp) 
        //    perror("pattern not found");
        
        return rest? rest : tk;
    }
            
    //size_t* begp = &range->beg;
    const char* rest = parse_range_addr(tk, tb, &range->beg);
    //if (rest && !*begp) 
    //    perror("pattern not found");
    
    if (rest) { 
        /* Addr... */
        tk = cstr_skip_space(rest);
        if (*tk == ',') {
            ++tk;
            /* Addr,... */
            //size_t end = 0;
            //size_t* endp = &range->end;
            rest = parse_range_addr(tk, tb, &range->end);
            //if (rest && !*endp) 
            //    perror("pattern not found");
            //else range->end = end;
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

const char*
parse_range(const char* tk, Range range[static 1], TextBuf tb[static 1]) {
    //size_t current_line, size_t nlines) {
    //size_t current_line, size_t nlines) {
    tk =  parse_range_impl(tk, tb, range);
    if (!tk || (!range_has_no_end(range) && is_range_valid(range, 0))) { return NULL; }
    else if (range->end == 0) range->end = range->beg; 
    return tk;
}

