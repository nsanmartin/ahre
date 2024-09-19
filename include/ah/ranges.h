#ifndef __AHED_RANGES_H__
#define __AHED_RANGES_H__

#include <stddef.h>

#include<ah/session.h>

typedef struct {
    size_t beg;
    size_t end;
    bool no_range;
} AeRange;

const char* ad_range_parse(const char* tk, Session session[static 1], AeRange* range);
#endif
