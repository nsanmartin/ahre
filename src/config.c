#include "config.h"
#include "bookmark.h"

#include "sys.h"


static bool _create_dir_(Str dirname[_1_]) {
    if (!len__(dirname)) return false;
    int rv = mkdir(items__(dirname), 0700);
    if (rv == 0) return true;
    return errno == EEXIST;
}

Err create_or_get_confdir(Str confdir[_1_]) {
    str_reset(confdir);
    char buf[PATH_MAX];
    char* xdg_config = NULL;
    char* xdg_config_env = getenv("XDG_CONFIG_HOME");
    char* basedir = NULL;

    if (xdg_config_env) {
        basedir = realpath(xdg_config_env, buf);
        if (basedir) {
            try (str_append_2(confdir, basedir, "/ahre"));
            if (_create_dir_(confdir)) return Ok;
        }
    }

    str_reset(confdir);
    char* home = getenv("HOME");
    if (home) {
        basedir = realpath(home, buf);
        if (basedir) {
            try (str_append_2(confdir, basedir, "/.config/ahre"));
            if (_create_dir_(confdir)) return Ok;

            str_reset(confdir);
            try (str_append_2(confdir, basedir, "/.ahre"));

            if (_create_dir_(confdir)) return Ok;
        }
    }

    if (xdg_config || home) return err_fmt("could not create confdir: %s", strerror(errno));
    return "could not create confdir, neither XDG_CONFIG_HOME not HOME env vars found";
}

Err resolve_bookmarks_file(const char* path, Str out[_1_]) {
    size_t prefix = len__(out);
    bool   file_exists;
    try(resolve_path(path, &file_exists, out));

    if (file_exists) return Ok;
    if (len__(out) <= prefix) {
        str_clean(out);
        return "error: prefix deleted while resolving path";
    }
    Err err = Ok;
    FILE* fp;
    tryjmp(err, Fail, file_open(items__(out) + prefix, "w", &fp));
    tryjmp(err, Fail, file_write_or_close(EMPTY_BOOKMARK, lit_len__(EMPTY_BOOKMARK), fp));
    tryjmp(err, Fail, file_close(fp));
    return Ok;
Fail:
    str_clean(out);
    return err;
}
