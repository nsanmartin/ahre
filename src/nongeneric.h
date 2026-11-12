/* Copyright (C) 2026 Nicolas San Martin

This file is part of ahre.

ahre is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

ahre is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ahre; see the file LICENSE or see
<https://www.gnu.org/licenses/>.  */

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
