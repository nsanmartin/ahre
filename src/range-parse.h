#ifndef AHED_RANGE_PARSE_H__
#define AHED_RANGE_PARSE_H__

#include "ranges.h"
#include "textbuf.h"


Err
parse_range( const char* tk, RangeParseResult res[static 1], const char* endptr[static 1]);
#endif
