#ifndef __AHED_RANGES_H__
#define __AHED_RANGES_H__

#include <stddef.h>

#include "src/textbuf.h"

typedef struct {
    size_t current_line;
    size_t nlines;
    bool not_found;
    TextBuf* tb;
} RangeParseCtx;


static inline RangeParseCtx
range_parse_ctx_from_textbuf(TextBuf tb[static 1]) {
    RangeParseCtx res = (RangeParseCtx){
        .current_line = *textbuf_current_line(tb),
        .nlines       = textbuf_line_count(tb),
        .not_found    = false,
        .tb           = tb
    };
    return res;
}

typedef struct { size_t beg; size_t end; } Range;

const char* 
parse_range(const char* tk, Range range[static 1], RangeParseCtx ctx[static 1]);
#endif
