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

#ifndef AHRE_CONFIG_H__
#define AHRE_CONFIG_H__

#include "str.h"
#include "utils.h"
#include "nongeneric.h"


//TODO: cache all these in session-conf

Err create_or_get_confdir(Str confdir[_1_]);

static inline Err get_config_dir(Str out[_1_]) {
    return create_or_get_confdir(out);
}

static inline Err append_cookies_filename(Str out[_1_]) {
#define COOKIES_FILENAME "cookies.txt"
    if (!len__(out)) return "config dir must not be empty";
    try( str_append(out, svl("/" COOKIES_FILENAME "\0")));
    return Ok;
}


static inline Err append_input_history_filename(Str out[_1_]) {
#define INPUT_HISTORY_FILENAME "input-history"
    if (!len__(out)) return "config dir must not be empty";
    try( str_append(out, svl("/" INPUT_HISTORY_FILENAME "\0")));
    return Ok;
}


static inline Err append_fetch_history_filename(Str out[_1_]) {
#define FETCH_HISTORY_FILENAME "fetch-history"
    if (!len__(out)) return "config dir must not be empty";
    try( str_append(out, svl("/" FETCH_HISTORY_FILENAME "\0")));
    return Ok;
}

static inline Err append_bookmark_filename(const char* fname, Str out[_1_]) {
    if (!len__(out)) return "config dir must not be empty";
    if (fname) {
        if (!*fname) return "invalid file name for bookmark (\"\")";
        return str_append_z(out, fname);
    }

    return str_append_z(out, svl("/" "bookmark.html"));
}
Err create_or_get_confdir(Str confdir[_1_]);

Err resolve_bookmarks_file(const char* path, Str out[_1_]);
#endif
