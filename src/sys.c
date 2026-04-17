#include "sys.h"
#include <sys/stat.h>

#include <limits.h>
#include <wordexp.h>


/* file utils */


Err resolve_path(const char *path, bool* file_exists, Str out[_1_]) {
    Str expanded = (Str){0};
    try(expand_path(path, &expanded));
    char buf[PATH_MAX];
    char *real = realpath(expanded.items, buf);
    if (!real && errno != ENOENT) {
    	str_clean(&expanded);
        return err_fmt("could not resolve path: %s", strerror(errno));
    }
    if (file_exists) *file_exists =  real && errno != ENOENT;
    Err err = Ok;
    if (real) err = str_append_z(out, sv(real, strlen(real)));
    else if (expanded.len) { err = str_append_z(out, expanded); }
    str_clean(&expanded);
    return err;
}



Err file_open(const char* file_path, const char* mode, FILE* fpp[_1_]) {
    if (!file_path) return "error: cannot open NULL file";
    if (!*file_path) return "cannot open empty string file";
    file_path = cstr_trim_space((char*)file_path);
    if (!*file_path) return "cannot open all whitespace file";
    Str path = (Str){0};
    try( resolve_path(file_path, NULL, &path));
    *fpp = fopen(path.items, mode);
    str_clean(&path);
    if (!*fpp)
        return err_fmt("error opening file '%s': %s", file_path, strerror(errno));
    return Ok;
}

Err _file_write_(const char* mem, size_t len, FILE* fp, const char* caller) {
    if (len && fwrite(mem, 1, len, fp) != len)
        return err_fmt("(%s) error writing to file: %s", caller, strerror(errno));
    return Ok;
} 

Err _file_write_sep_(
    const char* mem, size_t len, const char* sep, size_t seplen, FILE* fp, const char* caller
) {
    if (len && fwrite(mem, 1, len, fp) != len)
        return err_fmt("(%s) error writing to file: %s", caller, strerror(errno));
    if (seplen && fwrite(sep, 1, seplen, fp) != seplen)
        return err_fmt("(%s) error writing to file: %s", caller, strerror(errno));
    return Ok;
} 


Err _file_write_int_sep_(intmax_t i, const char* sep, size_t seplen, FILE* fp, const char* caller) {
    char buf [INT_TO_STR_BUFSZ];
    size_t len;
    try( int_to_str(i, buf, sizeof buf, &len));
    return _file_write_sep_(buf, len, sep, seplen, fp, caller);
}
                

Err _file_write_or_close_(const char* mem, size_t len, FILE* fp, const char* caller) {
    if (len && fwrite(mem, 1, len, fp) != len) {
        const char* fwrite_strerr = strerror(errno);
        bool close_err = fclose(fp);
        if (close_err) {
            const char* fclose_strerr = strerror(errno);
            return err_fmt(
                "error: both fwrite (%s) and fclose (%s) failed (%s)",
                fwrite_strerr, fclose_strerr, caller
            );
        }
        return err_fmt("error: fwrite failed: %s (%s)", fwrite_strerr, caller);
    }
    return Ok;
} 

Err file_close(FILE* fp) {
    if (fclose(fp)) return err_fmt("error closing file: %s", strerror(errno));
    return Ok;
}


Err expand_path(const char *path, Str out[_1_]) {
    wordexp_t result = {0};

    switch (wordexp (path, &result, WRDE_NOCMD | WRDE_UNDEF)) {
        case 0: break;
        case WRDE_BADVAL: 
            wordfree (&result);
            return "undefined enviroment variable in path";
        case WRDE_NOSPACE:
            wordfree (&result);
            return "error: out of memory parsing path";
        default: return "invalid path, wordexp could not parse";
    }

    if (result.we_wordc == 0) { wordfree(&result); return "invalid path: cannot be empty"; }
    const char* expanded = result.we_wordv[0];
    if (!strlen(expanded)) return "invalid path with no length";
    Err err = str_append_z(out, expanded);
    wordfree(&result);

    return err;
}

bool path_is_dir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
// file utils
