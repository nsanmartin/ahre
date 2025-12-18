#ifndef AHED_RANGE_PARSE_H__
#define AHED_RANGE_PARSE_H__

#include "ranges.h"
#include "textbuf.h"


Err
parse_range(const char* tk, RangeParse res[_1_], const char* endptr[_1_], int base);
#endif
