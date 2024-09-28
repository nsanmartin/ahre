#include "src/utils.h"
#include "src/user-interface.h"

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
    TextBuf* buf = session_current_buf(session);
    if (fname && *(fname = cstr_skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            return err_fmt("append: could not open file: %s", fname);
        }
        if (fwrite(textbuf_items(buf), 1, len(buf), fp) != len(buf)) {
            return err_fmt("append: error writing to file: %s", fname);
        }
        return 0;
    }
    return "append: fname missing";
}


/* ed cmds */
Err ed_write(const char* rest, TextBuf textbuf[static 1]) {
    if (!rest || !*rest) { return "cannot write without file arg"; }
    FILE* fp = fopen(rest, "w");
    if (!fp) {
        return err_fmt("%s: could not open file: %s", __func__, rest); 
    }
    if (fwrite(textbuf_items(textbuf), 1, len(textbuf), fp) != len(textbuf)) {
        return err_fmt("%s: error writing to file: %s", __func__, rest);
    }
    return Ok;
}


Err ed_global(TextBuf textbuf[static 1],  const char* rest) {
    (void)textbuf;
    Str pattern = parse_pattern(rest);
    if (!pattern.s || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.s);
    return Ok;
}

