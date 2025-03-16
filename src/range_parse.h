#ifndef AHED_RANGE_PARSE_H__
#define AHED_RANGE_PARSE_H__

#include "src/ranges.h"
#include "src/textbuf.h"

typedef const char* ParseRv;

typedef struct {
    size_t current_line;
    size_t nlines;
    TextBuf* tb;
} RangeParseCtx;


ParseRv parse_range(ParseRv tk, Range range[static 1], RangeParseCtx ctx[static 1]);
#define to_range_parse_err(Msg) ("\x01" Msg)
static inline bool range_parse_failure(ParseRv tk) { return !tk || *tk == '\x01'; }
static inline Err range_parse_failure_to_err(ParseRv tk) {
    return !tk
        ? "invalid range"
        : (*tk == '\x01' ? (tk + 1) : tk);
}

#define validate_parse_range_err(Value) _Generic((Value), ParseRv: Value)

#define try_parse_range(Parse) do{\
    Err ahre_parse_range_err_=validate_parse_range_err((Parse)); \
    if (range_parse_failure(ahre_parse_range_err_)) \
        return ahre_parse_range_err_;}while(0) 


inline static Err range_set_full(Range r[static 1], RangeParseCtx ctx[static 1]) {
    *r= (Range){.beg=1, .end=ctx->nlines};
    return Ok;
}

inline static Err range_beg_set_curr(Range r[static 1], RangeParseCtx ctx[static 1]) {
    r->beg = ctx->current_line;
    return Ok;
}

inline static Err range_both_set_curr(Range r[static 1], RangeParseCtx ctx[static 1]) {
    r->beg = ctx->current_line;
    r->end = r->beg;
    return Ok;
}

static inline RangeParseCtx
range_parse_ctx_from_textbuf(TextBuf tb[static 1]) {
    RangeParseCtx res = (RangeParseCtx){
        .current_line = textbuf_current_line(tb),
        .nlines       = textbuf_line_count(tb),
        .tb           = tb
    };
    return res;
}
#endif
