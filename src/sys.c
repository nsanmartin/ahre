#include "sys.h"

#include <limits.h>
#include <wordexp.h>



static Err expand_path(const char *path, Str out[_1_]);

Err resolve_path(const char *path, bool file_exists[_1_], Str out[_1_]) {
    Str expanded = (Str){0};
    try(expand_path(path, &expanded));
    char buf[PATH_MAX];
    char *real = realpath(expanded.items, buf);
    if (!real && errno != ENOENT) {
    	str_clean(&expanded);
        return err_fmt("could not resolve path: %s", strerror(errno));
    }
    *file_exists =  real && errno != ENOENT;
    Err err = Ok;
    if (real) err = str_append(out, sv(real, strlen(real) + 1));
    else err = str_append(out, expanded);
    str_clean(&expanded);
    return err;
}


/* internal linking */
static Err expand_path(const char *path, Str out[_1_]) {
    wordexp_t result = {0};

    switch (wordexp (path, &result, WRDE_NOCMD | WRDE_UNDEF)) {
        case 0: break;
        case WRDE_BADVAL: 
            wordfree (&result);
            return "undefined enviroment variable in path";
        case WRDE_NOSPACE:
            wordfree (&result);
            return "out of memory parsing path";
        default: return "invalid path, wordexp could not parse";
    }

    if (result.we_wordc == 0) { wordfree(&result); return "invalid path: cannot be empty"; }
    const char* expanded = result.we_wordv[0];
    if (!strlen(expanded)) return "invalid path with no length";
    Err err = str_append_z(out, expanded);
    wordfree(&result);

    return err;
}

