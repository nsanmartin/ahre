#ifndef AHRE_RANGES_H__
#define AHRE_RANGES_H__

#include <stddef.h>

typedef struct { size_t beg; size_t end; } Range;

inline static void range_end_set_beg(Range r[static 1]) { r->end = r->beg; }


#endif
