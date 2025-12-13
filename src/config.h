#ifndef AHRE_CONFIG_H__
#define AHRE_CONFIG_H__

#include "str.h"
#include "utils.h"

//TODO: cache all these in session-conf

static inline Err get_config_dir(Str out[_1_]) {
#define AHRE_CONFIG_SUBDIR "/.config/ahre"
    char* homedir = getenv("HOME");
    if (homedir) {
        try( str_append(out, homedir, strlen(homedir)));
        try( str_append_lit__(out, AHRE_CONFIG_SUBDIR));
        return Ok;
    }
    return "conf dir not found";
}

static inline Err get_cookies_filename(Str out[_1_]) {
#define COOKIES_FILENAME "cookies.txt"
    try( get_config_dir(out));
    try( str_append_lit__(out, "/" COOKIES_FILENAME "\0"));
    return Ok;
}


static inline Err get_input_history_filename(Str out[_1_]) {
#define INPUT_HISTORY_FILENAME "input-history"
    try( get_config_dir(out));
    try( str_append_lit__(out, "/" INPUT_HISTORY_FILENAME "\0"));
    return Ok;
}


static inline Err get_fetch_history_filename(Str out[_1_]) {
#define FETCH_HISTORY_FILENAME "fetch-history"
    try( get_config_dir(out));
    try( str_append_lit__(out, "/" FETCH_HISTORY_FILENAME "\0"));
    return Ok;
}

static inline Err get_bookmark_filename(const char* fname, Str out[_1_]) {
    if (fname) {
        if (!*fname) return "invalid file name for bookmark (\"\")";
        return str_append_z(out, (char*)fname, strlen(fname));
    }

    try( get_config_dir(out));
    return str_append_lit__(out, "/bookmark.html\0");
}

static inline Err get_bookmark_filename_if_it_exists(const char* fname, Str out[_1_]) {
#define FILE_SCHEMA "file://"
    try( get_bookmark_filename(fname, out));
    if ((str_startswith_lit(out, FILE_SCHEMA) && file_exists(items__(out) + lit_len__(FILE_SCHEMA)))
        || file_exists(items__(out))
    ) return Ok;
    Err err = err_fmt("No bookmarks file: '%s' cannot be accesed", items__(out));
    str_clean(out);
    return err;
}
Err resolve_bookmarks_file(const char* path, Str out[_1_]);
#endif
