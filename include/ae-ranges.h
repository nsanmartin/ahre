#ifndef __AHED_RANGES_H__
#define __AHED_RANGES_H__

#include <stddef.h>

typedef struct {
    size_t beg;
    size_t end;
} AeRange;

char* ad_range_parse(char* tk, AhCtx ctx[static 1], AeRange* range);
#endif
