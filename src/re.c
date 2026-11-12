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

#include "re.h"
#include <stdio.h>

#ifndef AHRE_REGEX_DISABLED


/* Err regex_search_next(const char* pattern, const char* string, size_t* off) { */

/*     regex_t regex; */
/*     if (regcomp(&regex, pattern, REG_NEWLINE)) */
/*         return err_fmt("error compiling regex pattern: %s", pattern); */

/*     regmatch_t  pmatch[1]; */
/*     int status = regexec(&regex, string, 1, pmatch, 0); */
/*     if (status == REG_NOMATCH) { */
/*         return "pattern not found"; */
/*     } else if (status) { */
/*         return err_fmt("error executing regex, status: %d\n", status); */
/*     } */

/*     *off = pmatch[0].rm_so; */

/*     regfree(&regex); */

/*     return Ok; */
/* } */

Err
regex_maybe_find_next(const char* pattern, const char* string, size_t* off[1]) {

    regex_t regex;
    if (regcomp(&regex, pattern, REG_NEWLINE))
        return err_fmt("error compiling regex pattern: %s", pattern);

    regmatch_t  pmatch[1];
    int status = regexec(&regex, string, 1, pmatch, 0);
    if (status == REG_NOMATCH) {
        *off = NULL;
        regfree(&regex);
        return Ok;
    } else if (status) {
        regfree(&regex);
        *off = NULL;
        return err_fmt("error executing regex, status: %d\n", status);
    }

    **off = pmatch[0].rm_eo;

    regfree(&regex);

    return Ok;
}

#else /* regex disabled: */
typedef int _regex_disabled_;
#endif /* AHRE_REGEX_DISABLED */
