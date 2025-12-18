#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "utils.h"
#include "textbuf.h"
#include "range-parse.h"
#include "re.h"
#include "str.h"

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

_Static_assert(sizeof(size_t) <= sizeof(uintmax_t), "range parser requires this precondition");


static Err
_parse_range_addr_delta_(const char* tk, RangeAddr out[_1_], const char* endptr[_1_]) {
    tk = cstr_skip_space(tk);
    if (*tk == '+' || *tk == '-') {
        //TODO: what if "+  7778"?
        intmax_t i;
        try( parse_ll_err(tk, &i, endptr));
        if (i > INT_MAX) { return "error: relative rnge too large"; }
        if (tk == *endptr) {
            out->delta = *tk == '-' ? -1 : 1;
            *endptr = tk + 1;
        } else out->delta = i;
        return Ok;
    }
    out->delta = 0;
    *endptr    = tk;
    return Ok;
}


static Err
_parse_range_addr_(const char* tk, RangeAddr out[_1_], const char* endptr[_1_], int base) {
    if (base != 10 && base != 36) return "error: onle base 10 and base 36 supported";
    if (!tk) return "error: cannot parse NULL";
    *endptr = tk;
    *out = (RangeAddr){.tag=range_addr_none_tag};
    if (!**endptr) return Ok;
    if (**endptr == '/') {
        *out = (RangeAddr) { .tag = range_addr_search_tag };
        ++*endptr;
        char* end = strchr(*endptr, '/');
        if (end) {
            size_t l = 1 + (end - *endptr);
            out->s = (StrView){.items=*endptr, .len=l};
            return _parse_range_addr_delta_(*endptr+l, out, endptr);
        } else {
            size_t l = strlen(*endptr);
            out->s = (StrView){.items=*endptr, .len=l+1};
            *endptr = *endptr + l;
            return Ok;
        }
    } else if (**endptr == '.' || **endptr == '+' || **endptr == '-')  {
        *out = (RangeAddr) { .tag = range_addr_curr_tag };
        if (**endptr == '.') *endptr = cstr_skip_space(*endptr+1);
        if (**endptr == '+' || **endptr == '-')
            return _parse_range_addr_delta_(*endptr, out, endptr);
        return Ok;

    } else if (**endptr == '$') {
        *out = (RangeAddr) { .tag = range_addr_end_tag };
        *endptr = *endptr + 1;
        return _parse_range_addr_delta_(*endptr, out, endptr);
    } else if ((base == 10 && isdigit(**endptr)) || (base == 36 && isalnum(**endptr))) {
        /* *endptr does not start neither with - nor + */
        *out = (RangeAddr) { .tag = range_addr_num_tag };
        try( parse_size_t_err(*endptr, &out->n, endptr, base)); //no need to check tk != endptr
        *endptr = cstr_skip_space(*endptr);
        if (**endptr == '+' || **endptr == '-')
            return _parse_range_addr_delta_(*endptr, out, endptr);
        return Ok;
    }

    if (**endptr == '+' || **endptr == '-') {
        *out = (RangeAddr) { .tag = range_addr_curr_tag };
        return _parse_range_addr_delta_(*endptr, out, endptr);
    }
    *endptr = tk;
    return Ok;
}


/* external linkage */


Err parse_range(
    const char* tk,
    RangeParse  res[_1_],
    const char* endptr[_1_],
    int         base
) {
    if (!tk) return "Error: cannot parse NULL, invalid input";

    tk = cstr_skip_space(tk);

    if (!*tk) { /* empty string */
        *endptr = tk;
        *res = (RangeParse) {
            .beg={.tag=range_addr_none_tag},
            .end={.tag=range_addr_none_tag}
        };
        return Ok;
    }

    if (*tk == '^') {
        *endptr = cstr_skip_space(tk + 1);
        *res = (RangeParse) {
            .beg={.tag=range_addr_prev_tag},
            .end={.tag=range_addr_prev_tag}
        };
        return Ok;
    }

    if (*tk == '%') { /* full range */
        *endptr = cstr_skip_space(tk + 1);
        *res = (RangeParse) {
            .beg={.tag=range_addr_beg_tag},
            .end={.tag=range_addr_end_tag}
        };
        return Ok;
    }

    /* ,Addr? */
    if (*tk == ',') {
        ++tk;
        *res = (RangeParse) { .beg={.tag=range_addr_curr_tag, .n=1}, };
        return _parse_range_addr_(tk, &res->end, endptr, base);
    }

    *res = (RangeParse) {0};
    try (_parse_range_addr_(tk, &res->beg, endptr, base));
    if (tk == *endptr /* no_parse */) {
        /* No range was giving so defaulting to current line.
         * This is the case of e.g. 'p' that prints current line.
         */
        *res = (RangeParse) {
            .beg={.tag=range_addr_none_tag},
            .end={.tag=range_addr_none_tag}
        };
        return Ok;
    }

    /* Addr... */
    tk = cstr_skip_space(*endptr);
    if (*tk == ',') {
        ++tk;
        /* Addr,... */
        return _parse_range_addr_(tk, &res->end, endptr, base);
    } else {
        /* AddrEORange */
        res->end = (RangeAddr){.tag=range_addr_none_tag};
        return Ok;
    }

}
