#include <ah/utils.h>
#include <ah/wrappers.h>
#include <ah/user-interface.h>

Str parse_pattern(const char* tk) {
    Str res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) {
        res = (Str){.s = tk, .len=strlen(tk)};
    } else {
        res = (Str){.s = tk, .len=end-tk};
    }
    return res;
}

/* ah cmds */

ErrStr cmd_write(const char* fname, Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (fname && *(fname = skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            fprintf(stderr, "append: could not open file: %s\n", fname);
            return  "append: could not open file";
        }
        if (fwrite(buf->items, 1, buf->len, fp) != buf->len) {
            fprintf(stderr, "append: error writing to file: %s\n", fname);
            return "append: error writing to file";
        }
        return 0;
    }
    puts("append: fname missing");
    return "append: fname missing";
}


/* ed cmds */
ErrStr ed_write(const char* rest, Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (!rest || !*rest) { return "cannot write without file arg"; }
    FILE* fp = fopen(rest, "w");
    if (!fp) {
        fprintf(stderr, "write: could not open file: %s\n", rest); 
        return  "write: could not open file.";
    }
    if (fwrite(buf->items, 1, buf->len, fp) != buf->len) {
        fprintf(stderr, "write: error writing to file: %s\n", rest);
        return "write: error writing to file.";
    }
    return NULL;
}


ErrStr ed_global(Session session[static 1],  const char* rest) {
    (void)session;
    Str pattern = parse_pattern(rest);
    if (!pattern.s || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.s);
    return NULL;
}



