#ifndef __AHED_RANGES_H__
#define __AHED_RANGES_H__

#include <stddef.h>

#include "src/session.h"

typedef struct { size_t beg; size_t end; } Range;

const char* parse_range(const char* tk, Range range[static 1], size_t current_line, size_t nlines);
#endif
