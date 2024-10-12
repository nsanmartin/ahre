#ifndef __AHED_RANGES_H__
#define __AHED_RANGES_H__

#include <stddef.h>

#include "src/textbuf.h"


typedef struct { size_t beg; size_t end; } Range;

const char* 
parse_range(const char* tk, Range range[static 1], TextBuf tb[static 1]);
#endif
