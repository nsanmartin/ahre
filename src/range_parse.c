#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include "src/utils.h"
#include "src/textbuf.h"
#include "src/range_parse.h"
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

static const char*
parse_range_addr_num(const char* tk, uintmax_t num, uintmax_t maximum, size_t* out) {
    uintmax_t ull;
    if (*tk == '+') {
        ++tk;
        const char* rest = parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        else { tk = rest; }
        num += ull; 
        if (num < ull) { return to_range_parse_err("error: unsigned overflow reading range"); }
        if (num > maximum) { return to_range_parse_err("invalid range end (too large)"); }
    } else if (*tk == '-') {
        ++tk;
        const char* rest = parse_ull(tk, &ull);
        if (!rest) { ull = 1; }
        else { tk = rest; }
        if (ull > num) { return to_range_parse_err("error: ull > num"); }
        num -= ull; 
        if (num > maximum) { return to_range_parse_err("invalid range end (too large)"); }
    }
    *out = num;
    return tk;
}

_Static_assert(sizeof(size_t) <= sizeof(uintmax_t), "range parser requires this precondition");

static ParseRv
parse_linenum_search_regex(ParseRv tk, TextBuf tb[static 1], size_t out[static 1]) {
    const char* buf = textbuf_current_line_offset(tb);
    if (!buf) { return to_range_parse_err("error: invalid current line"); }
    char* end = strchr(tk, '/');
    const char* res = end ? end + 1 : tk + strlen(tk);
    if (end) *end = '\0';
    size_t match = 0;
    size_t* pmatch = &match;
    Err err = regex_maybe_find_next(tk, buf, &pmatch);
    if (end) *end = '/';
    if (err) return err_prepend_char(err, '\x01');

    if (pmatch)  {
        try ((err = textbuf_get_line_of(tb, buf + match, out)));
    } else return to_range_parse_err("pattern not found"); 

    return res;
}

static ParseRv
parse_range_addr(const char* tk, RangeParseCtx ctx[static 1], size_t out[static 1]) {

    ParseRv const no_parse = tk;
    ParseRv rest;
    uintmax_t curr = ctx->current_line;
    uintmax_t max  = ctx->nlines;
    if (!tk) { return NULL; }
    if (!*tk) { return tk; }
    if (*tk == '/') {
        ++tk;
        return parse_linenum_search_regex(tk, ctx->tb, out);
    } else if (*tk == '.' || *tk == '+' || *tk == '-')  {
        if (*tk == '.') { ++tk; }
        try_parse_range((rest = parse_range_addr_num(tk, curr, max, out)));
        return rest;
    } else if (*tk == '$') {
        ++tk;
        try_parse_range((rest=parse_range_addr_num(tk, max, max, out)));
        return rest;
    } else if (isdigit(*tk)) {
        /* tk does not start neither with - nor + */
        try_parse_range((rest = parse_ull(tk, &curr)));
        if (curr > max) { return to_range_parse_err("invalid range (num too large)"); }
        *out = curr;
        if (*rest == '+' || *rest == '-') {
            try_parse_range((rest = parse_range_addr_num(rest+1, curr, max, out)));
        }
        return rest;
    } 

    if (*tk == '+' || *tk == '-') {
        try_parse_range((rest = parse_range_addr_num(tk+1, curr, max, out)));
        return rest;
    }
    return no_parse;
}


static const char*
parse_range_impl(const char* tk, RangeParseCtx ctx[static 1], Range range[static 1]) {

    if (!tk) return NULL; /* invalid input */

    ParseRv const no_parse = tk;
    tk = cstr_skip_space(tk);

    if (!*tk) return no_parse; /* empty string */

    if (*tk == '^') {
        *range = *textbuf_last_range(ctx->tb);
        if (range->end == 0) { return err_fmt("\x01no last range"); }
        return tk + 1;
    }

    /* full range */
    if (*tk == '%') {
        ++tk;
        range_set_full(range, ctx);
        return tk;
    } 

    ParseRv rest;
    /* ,Addr? */
    if (*tk == ',') {
        ++tk;
        range_beg_set_curr(range, ctx);
        try_parse_range((rest = parse_range_addr(tk, ctx, &range->end)));
        return rest;
    }

    try_parse_range((rest = parse_range_addr(tk, ctx, &range->beg)));
    if (tk == rest /* no_parse */) {
        /* No range was giving so defaulting to current line.
         * This is the case of e.g. 'p' that prints current line.
         */
        range_both_set_curr(range, ctx);
        return rest;
    }
    
    /* Addr... */
    tk = cstr_skip_space(rest);
    if (*tk == ',') {
        ++tk;
        /* Addr,... */
        try_parse_range((rest = parse_range_addr(tk, ctx, &range->end)));
        if (tk == rest /* no_parse */) range_end_set_beg(range);
        return rest;
    } else {
        /* AddrEORange */
        range_end_set_beg(range);
        return tk;
    }

}

/* external linkage */

ParseRv
parse_range(const char* tk, Range range[static 1], RangeParseCtx ctx[static 1]) {
    ParseRv parse_str =  parse_range_impl(tk, ctx, range);
    if (range_parse_failure(parse_str)) return parse_str;
    if (tk == parse_str /* no_parse */) {
        range_both_set_curr(range, ctx);
        return parse_str;
    }
    if (range->beg == 0 || range->end == 0) return to_range_parse_err("Zero not supported in ranges");
    if (range->end < range->beg) return to_range_parse_err("backward range");

    if (textbuf_line_count(ctx->tb) < range->end)
        return to_range_parse_err("error: not expecting invalid range end");
    return parse_str;
}

