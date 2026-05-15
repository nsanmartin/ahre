/*
 * Header file that do not define any generic. 
 * It may be included in .h files
 */
#ifndef __NONGENERIC_AHRE_H__
#define __NONGENERIC_AHRE_H__


#define len__(Ptr) (Ptr)->len
#define items__(Ptr) (Ptr)->items

#include <stddef.h>
static inline size_t min_size_t(size_t x, size_t y) { return x < y ? x : y; }
static inline unsigned min_unsigned(unsigned x, unsigned y) { return x < y ? x : y; }

#endif
