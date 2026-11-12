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

#ifndef __MEM_AHRE_H__
#define __MEM_AHRE_H__

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#include <stdlib.h>
#include <limits.h>
#include <string.h>


#define std_strdup  strdup
#define std_free    free
#define std_malloc  malloc
#define std_calloc  calloc

#ifdef AHRE_SIMULATE_REALLOC_ERROR
void* realloc_simulate_error(void* ptr, size_t n);
#define std_realloc realloc_simulate_error
#else
#define std_realloc realloc
#endif

#endif
