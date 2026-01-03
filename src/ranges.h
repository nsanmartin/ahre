#ifndef AHRE_RANGES_H__
#define AHRE_RANGES_H__

#include <stddef.h>
#include "str.h"

typedef struct { size_t beg, end; } Range;

typedef enum {
    range_addr_none_tag = 0,
    range_addr_beg_tag,
    range_addr_curr_tag,
    range_addr_end_tag,
    range_addr_num_tag,
    range_addr_search_tag,
    range_addr_prev_tag
} RangeAddrTag;

typedef struct {
    RangeAddrTag tag;
    union {
        size_t  n;
        StrView s;
    };
    int delta;
} RangeAddr;


typedef struct { RangeAddr beg, end; } RangeParse;

inline static void range_end_set_beg(Range r[_1_]) { r->end = r->beg; }

inline static bool range_parse_is_none(RangeParse rp[_1_]) {
    return rp->beg.tag == range_addr_none_tag && rp->end.tag == range_addr_none_tag;
}

static inline Err basic_size_t_from_addr(
    RangeAddr addr[_1_],
    size_t min,
    size_t bound,
    size_t const* optional_none_default,
    size_t out[_1_]
) {
    if (!bound) return "invalid range for bound=0";
    switch(addr->tag) {
        case range_addr_beg_tag: *out=min; return Ok;
        case range_addr_end_tag: *out=bound - 1; return Ok;
        case range_addr_num_tag: *out=addr->n; return Ok;
        case range_addr_none_tag:
            if (optional_none_default) {
                *out=*optional_none_default;
                return Ok;
            }
            return "no default value for range type";
        default: return "error parsing addr";
    }
}

static inline Err
basic_range_from_parse(RangeParse rp[_1_], size_t min, size_t bound, Range out[_1_]) {
    try(basic_size_t_from_addr(&rp->beg, min, bound, NULL, &out->beg));
    try(basic_size_t_from_addr(&rp->end, min, bound, &out->beg, &out->end));
    if (out->beg > out->end) return "invalid range end <= beg";
    ++out->end;
    return Ok;
}
#endif
