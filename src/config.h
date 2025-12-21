#ifndef AHRE_CONFIG_H__
#define AHRE_CONFIG_H__

#include "str.h"
#include "utils.h"

//TODO: cache all these in session-conf

Err create_or_get_confdir(Str confdir[_1_]);

static inline Err get_config_dir(Str out[_1_]) {
    return create_or_get_confdir(out);
}

static inline Err append_cookies_filename(Str out[_1_]) {
#define COOKIES_FILENAME "cookies.txt"
    if (!len__(out)) return "config dir must not be empty";
    try( str_append_lit__(out, "/" COOKIES_FILENAME "\0"));
    return Ok;
}


static inline Err append_input_history_filename(Str out[_1_]) {
#define INPUT_HISTORY_FILENAME "input-history"
    if (!len__(out)) return "config dir must not be empty";
    try( str_append_lit__(out, "/" INPUT_HISTORY_FILENAME "\0"));
    return Ok;
}


static inline Err append_fetch_history_filename(Str out[_1_]) {
#define FETCH_HISTORY_FILENAME "fetch-history"
    if (!len__(out)) return "config dir must not be empty";
    try( str_append_lit__(out, "/" FETCH_HISTORY_FILENAME "\0"));
    return Ok;
}

static inline Err append_bookmark_filename(const char* fname, Str out[_1_]) {
    if (!len__(out)) return "config dir must not be empty";
    if (fname) {
        if (!*fname) return "invalid file name for bookmark (\"\")";
        return str_append_z(out, (char*)fname, strlen(fname));
    }

    return str_append_lit__(out, "/" "bookmark.html" "\0");
}
Err create_or_get_confdir(Str confdir[_1_]);

Err resolve_bookmarks_file(const char* path, Str out[_1_]);
#endif
