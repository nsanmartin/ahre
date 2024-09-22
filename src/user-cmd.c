#include <ah/utils.h>
#include <ah/wrappers.h>
#include <ah/user-interface.h>

Str parse_pattern(const char* tk) {
    Str res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = cstr_skip_space(tk);
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

Err cmd_write(const char* fname, Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (fname && *(fname = cstr_skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            return err_fmt("append: could not open file: %s", fname);
        }
        if (fwrite(buf->items, 1, buf->len, fp) != buf->len) {
            return err_fmt("append: error writing to file: %s", fname);
        }
        return 0;
    }
    return "append: fname missing";
}


/* ed cmds */
Err ed_write(const char* rest, Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (!rest || !*rest) { return "cannot write without file arg"; }
    FILE* fp = fopen(rest, "w");
    if (!fp) {
        return err_fmt("%s: could not open file: %s", __func__, rest); 
    }
    if (fwrite(buf->items, 1, buf->len, fp) != buf->len) {
        return err_fmt("%s: error writing to file: %s", __func__, rest);
    }
    return Ok;
}


Err ed_global(Session session[static 1],  const char* rest) {
    (void)session;
    Str pattern = parse_pattern(rest);
    if (!pattern.s || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.s);
    return Ok;
}

