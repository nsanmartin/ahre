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

#ifndef __AHRE_RE_H__
#define __AHRE_RE_H__

#include "error.h"
#include "utils.h"

#ifndef AHRE_REGEX_DISABLED

#include <regex.h>

Err regex_maybe_find_next(
    const char* pattern,
    const char* string,
    size_t* off[_1_]
);

#else
#define regex_maybe_find_next(P, S, M) "warn: regex not supported in this build"
#endif /* AHRE_REGEX_DISABLED */

#endif /* __AHRE_RE_H__ */
