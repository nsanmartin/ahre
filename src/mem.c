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

#include "mem.h"


#ifdef AHRE_SIMULATE_REALLOC_ERROR
void* realloc_simulate_error(void* ptr, size_t n) {
    static size_t counter = 0;
    if (counter++ % 5 == 0) return NULL;
    return realloc(ptr, n);
}
#endif
