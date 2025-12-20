#include <sys/stat.h>
#include "config.h"


static Err _str_concat_mem_2z(char* m1, size_t l1, char* m2, size_t l2, Str out[_1_]) {
    try(str_append(out, m1, l1));
    try(str_append(out, m2, l2));
    return str_append_lit__(out, "\0");
}
#define _str_concat_mem_lit_z__(M1, L1, Lit, Out) \
    _str_concat_mem_2z(M1, L1, Lit, lit_len__(Lit), Out)

static bool _create_dir_and_remove_z_(Str dirname[_1_]) {
    int rv = mkdir(items__(dirname), 0700);
    --dirname->len;
    if (!len__(dirname)) return false;
    if (rv == 0) return true;
    return errno == EEXIST;
}

Err create_or_get_confdir(Str confdir[_1_]) {
    str_reset(confdir);
    char* xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config) {
        try (_str_concat_mem_lit_z__(xdg_config, strlen(xdg_config), "/ahre", confdir));
        if (_create_dir_and_remove_z_(confdir)) return Ok;
    }

    str_reset(confdir);
    char* home = getenv("HOME");
    if (home) {
        try (_str_concat_mem_lit_z__(home, strlen(home), "/.config/ahre", confdir));
        if (_create_dir_and_remove_z_(confdir)) return Ok;

        str_reset(confdir);
        try (_str_concat_mem_lit_z__(home, strlen(home), "/.ahre", confdir));

        if (_create_dir_and_remove_z_(confdir)) return Ok;
    }

    if (xdg_config || home) return err_fmt("could not create confdir: %s", strerror(errno));
    return "could not create confdir, neither XDG_CONFIG_HOME not HOME env vars found";
}

Err resolve_bookmarks_file(const char* path, Str out[_1_]) {
    bool file_exists;
    try(resolve_path(path, &file_exists, out));

    if (file_exists) return Ok;
    Err err = Ok;
#define EMPTY_BOOKMARK \
    "<html><head><title>Bookmarks</title></head>\n<body>\n<h1>Bookmarks</h1>\n</body>\n</html>"
    FILE* fp;
    try_or_jump(err, Fail, file_open(items__(out), "w", &fp));
    try_or_jump(err, Fail, file_write_or_close(EMPTY_BOOKMARK, lit_len__(EMPTY_BOOKMARK), fp));
    try_or_jump(err, Fail, file_close(fp));
    return Ok;
Fail:
    str_clean(out);
    return err;
}
