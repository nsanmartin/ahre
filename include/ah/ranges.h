#ifndef __AHED_RANGES_H__
#define __AHED_RANGES_H__

#include <stddef.h>

#include<ah/session.h>

typedef struct { size_t beg; size_t end; } Range;

const char* range_parse(const char* tk, Session session[static 1], Range* range);
#endif
