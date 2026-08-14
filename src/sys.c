#include "sys.h"
#include "generic.h"
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>


#include <limits.h>
#include <wordexp.h>

static bool is_wordexp_failure(Err e);


/* file utils */


Err resolve_path(const char *path, bool* file_exists, Str out[_1_]) {
    if (file_exists) *file_exists = false;
    Err err      = Ok;
    Str expanded = (Str){0};
    err = expand_path(path, &expanded);
    if (is_wordexp_failure(err)) {
        err = Ok;
        tryjmp(err, Clean, str_append_z(&expanded, sv(path)));
    }
    else if (err) { goto Clean; }
    char buf[PATH_MAX];
    char *real = realpath(expanded.items, buf);
    if (!real && errno != ENOENT) {
        err=err_fmt("could not resolve path: %s", strerror(errno));
        goto Clean;
    }
    if (file_exists) *file_exists =  real && errno != ENOENT;
    if (real) err = str_append_z(out, sv(real, strlen(real)));
    else if (expanded.len) { err = str_append_z(out, expanded); }

Clean:
    str_clean(&expanded);
    return err;
}



Err file_open(const char* file_path, const char* mode, FILE* fpp[_1_]) {
    if (!file_path) return "error: cannot open NULL file";
    if (!*file_path) return "cannot open empty string file";
    file_path = cstr_trim_space((char*)file_path);
    if (!*file_path) return "cannot open all whitespace filename";
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
    if (!fp) return Ok;
    if (fclose(fp)) return err_fmt("error closing file: %s", strerror(errno));
    return Ok;
}

#define WRDE_FAIL_MSG    "wordexp failure: "
#define WRDE_BADVAL_MSG  "undefined enviroment variable in path"
#define WRDE_NOSPACE_MSG "error: out of memory parsing path"
#define WRDE_CMDSUB_MSG  "Command substitution requested, but the WRDE_NOCMD flag told us to consider this an error."
#define WRDE_SYNTAX_MSG  "Shell syntax error, such as unbalanced parentheses or unmatched quotes."
#define WRDE_BADCHAR_MSG "Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, }."

#define WRDE_SUBMSG_MAX_LEN sizeof(WRDE_CMDSUB_MSG) - 1
#define WRDE_FAIL_MSG_LEN sizeof(WRDE_FAIL_MSG) - 1

static char wordexp_failure__[WRDE_FAIL_MSG_LEN + WRDE_SUBMSG_MAX_LEN + 1] = { WRDE_FAIL_MSG };
static Err
wordexp_failure(StrView msg) { 
    if (msg.len > WRDE_SUBMSG_MAX_LEN) return err_internal("invalid wordexp failure msg");
    char* buffer = wordexp_failure__ + lit_len__(WRDE_FAIL_MSG);
    if (msg.len) memcpy(buffer, msg.items, msg.len);
    buffer[msg.len] = '\0';
    return wordexp_failure__;
}

Err expand_path(const char *path, Str out[_1_]) {
    wordexp_t result = {0};
    int rv = wordexp (path, &result, WRDE_NOCMD | WRDE_UNDEF);
    switch (rv) {
        case 0: break;
        case WRDE_BADVAL: 
            wordfree (&result);
            return wordexp_failure(svl(WRDE_BADVAL_MSG));
        case WRDE_NOSPACE:
            wordfree (&result);
            return wordexp_failure(svl(WRDE_NOSPACE_MSG));
        case WRDE_CMDSUB:
            return wordexp_failure(svl(WRDE_CMDSUB_MSG));
        case WRDE_SYNTAX:
            return wordexp_failure(svl(WRDE_SYNTAX_MSG));
        case WRDE_BADCHAR:
            return wordexp_failure(svl(WRDE_BADCHAR_MSG));

        default: return "invalid path, wordexp could not parse";
    }

    if (result.we_wordc == 0) { wordfree(&result); return "invalid path: cannot be empty"; }
    const char* expanded = result.we_wordv[0];
    if (!strlen(expanded)) return "invalid path with no length";
    Err err = str_append_z(out, expanded);
    wordfree(&result);

    return err;
}

static bool
is_wordexp_failure(Err e) { return e == wordexp_failure__; }


bool path_is_dir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


Err
file_read(FilePtr fp[1], char* mem, size_t len, size_t read[1]) {
    *read = fread(mem, 1, len, fp->ptr);
    if (ferror(fp->ptr)) return err_from(errno);
    if (*read < len && !feof(fp->ptr)) return err_from("fread read less but did not read eof?");
    return Ok;
}

// file utils


static volatile sig_atomic_t interrupt_flag__ = 0;
bool interrupt_flag(void) {
    bool res = interrupt_flag__ != 0;
    interrupt_flag__ = 0;
    return res;
}
static void set_interrupt_flag(int sig) {
    (void)sig;
    interrupt_flag__ = 1;
}

struct sigaction get_interrupt_action(void) {
    struct sigaction res = (struct sigaction){.sa_handler=set_interrupt_flag};
    sigemptyset(&res.sa_mask);
    return res;
}

Err
append_fnames_from_dir(const char* dir_path, ArlOf(Str) fnames[1]) {
    if (!dir_path || !*dir_path) return "invalid empty path";

    DIR *dir = opendir(dir_path);

    if (!dir) return err_fmt("warn: %s", strerror(errno));

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            continue;

        Str* sptr = NULL;
        try(arl_append_zero(Str, fnames, sptr));
        try(str_append(sptr, sv(dir_path)));
        try(str_append(sptr, svl("/")));
        try(str_append(sptr, sv(entry->d_name)));

        if (path_is_dir(sptr->items)) arlfn(Str,pop)(fnames);
    }

    closedir(dir);
    return Ok;
}
