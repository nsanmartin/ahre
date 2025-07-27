#ifndef AHRE_CONFIG_H__
#define AHRE_CONFIG_H__

#include "utils.h"

//TODO: cache all these in session-conf

static inline Err get_config_dir(BufOf(char)* out) {
#define AHRE_CONFIG_SUBDIR "/.config/ahre"
    char* homedir = getenv("HOME");
    if (homedir) {
        if (!buffn(char, append)(out, homedir, strlen(homedir))
          ||!buffn(char, append)(out, AHRE_CONFIG_SUBDIR, lit_len__(AHRE_CONFIG_SUBDIR))
        ) return  "error: buf append failure";
        return Ok;
    }
    return "conf dir not found";
}

static inline Err get_cookies_filename(BufOf(char)* out) {
# define COOKIES_FILENAME "/cookies.txt"
    try( get_config_dir(out));
    if (!buffn(char, append)(out, COOKIES_FILENAME, lit_len__(COOKIES_FILENAME))
      ||!buffn(char, append)(out, "\0", 1)
    ) return "error: buf append failure";
    return Ok;
}

static inline Err get_bookmark_filename(BufOf(char)* out) {
    try( get_config_dir(out));
    if (!buffn(char, append)(out, "/bookmark.html", sizeof("/bookmark.html")-1)
      ||!buffn(char, append)(out, "\0", 1)
    ) return "error: buf append failure";
    return Ok;
}

static inline Err get_bookmark_filename_if_it_exists(Str* out) {
    try( get_bookmark_filename(out));
    if (file_exists(items__(out))) return Ok;
    Err err = err_fmt("No bookmarks file: '%s' cannot be accesed", items__(out));
    str_clean(out);
    return err;
}
#endif
