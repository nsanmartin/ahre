#ifndef __GENERIC_AHRE_H__
#define __GENERIC_AHRE_H__
#include <stddef.h>


#define len__(Ptr) (Ptr)->len
#define items__(Ptr) (Ptr)->items

static inline size_t min_size_t(size_t x, size_t y) { return x < y ? x : y; }

#define min(X, Y) _Generic((X),\
    size_t:_Generic((Y), size_t:min_size_t)\
    )((X),(Y))


#endif
