#ifndef AHRE_CONFIG_H__
#define AHRE_CONFIG_H__

#include "src/utils.h"

static inline Err get_config_dir(BufOf(char)* out) {
    char* homedir = getenv("HOME");
    if (homedir) {
        if (!buffn(char, append)(out, homedir, strlen(homedir))
                //TODO: use .config/ahre or something similar
          ||!buffn(char, append)(out, "/.w3m", sizeof("/.w3m")-1)
        ) return  "error: buf append failure";
        return Ok;
    }
    return "conf dir not found";
}

static inline Err get_bookmark_filename(BufOf(char)* out) {
    //if (!buffn(char, append)(out, "file://", sizeof("file://")-1)) return "error: buf append failure";
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
