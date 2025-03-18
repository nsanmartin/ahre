#include "src/user-out-vi-mode.h"
#include "src/cmd-ed.h"
#include "src/user-out.h"
#include "src/session.h"

void _update_if_smaller_(size_t value[static 1], size_t new_value) {
    if (*value > new_value) *value = new_value;
}

Err _vi_show_session_(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";
    if (session_is_empty(s)) {
        session_uout(s)->write_std("Session is empty", lit_len__("Session is empty"), s);
        session_uout(s)->flush_std(s);
        return Ok;
    }

    session_uout(s)->flush_msg(s);

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    _update_if_smaller_(session_nrows(s), nrows - 2);
    _update_if_smaller_(session_ncols(s), ncols);

    if (nrows <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb);
    if (!line) return "error: expecting line number, not found";
    size_t end = line + *session_nrows(s);
    end = (end > textbuf_line_count(tb)) ? textbuf_line_count(tb) : end;
    Range r = (Range){ .beg=line, .end=end };

    try( _vi_print_(tb, &r, s));
    return Ok;
}


Err _vi_print_(TextBuf textbuf[static 1], Range range[static 1], Session* s) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        if (!line.items || !*line.items) continue; //TODO: fix get-line

        if (true) {
            if (line.len) { try( session_write_std(s, (char*)line.items, line.len)); }
            else try( session_write_std_lit__(s, "\n"));
        } else {
            try( session_write_unsigned_std(s, linum));
            try( session_write_std_lit__(s,"\t"));
            if (line.len) { try( session_write_std(s, (char*)line.items, line.len)); }
            else try( session_write_std_lit__(s, "\n"));
        }
    }
    return Ok;
}

Err _vi_write_msg_(const char* mem, size_t len, Session* s) {
    if (!s) return "error: no session";
    Str* buf = session_msg(s);
    return str_append(buf, (char*)mem, len);
}

#define CONTINUE_MSG_ "{% type any key to continue %}"
Err _vi_flush_msg_(Session* s) {
    if (!s) return "error: no session";
    Str* msg = session_msg(s);
    if (!len__(msg)) return Ok;
    if (len__(msg) != fwrite(items__(msg), 1, len__(msg), stdout)) return "error: fwrite failure";
    if (len__(msg) != fwrite(CONTINUE_MSG_, 1, lit_len__(CONTINUE_MSG_), stdout))
        return "error: fwrite failure";
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    getchar();
    str_reset(msg);
    return Ok;
}

Err _vi_show_err_(Session* s, char* err, size_t len) {
    (void)s;
    if (err) {
        if (len != fwrite(err, 1, len, stderr))
            return "error: fprintf failure while attempting to show an error :/";
    }
    fflush(stderr);
    getchar();
    return Ok;
}
