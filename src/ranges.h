#ifndef AHRE_RANGES_H__
#define AHRE_RANGES_H__

#include <stddef.h>
#include "str.h"

typedef struct { size_t beg, end; } Range;

typedef enum {
    range_addr_curr_tag,
    range_addr_end_tag,
    range_addr_none_tag,
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


typedef struct { RangeAddr beg, end; } RangeParseResult;

inline static void range_end_set_beg(Range r[static 1]) { r->end = r->beg; }


#endif
