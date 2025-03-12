#include "src/user-out-vi-mode.h"
#include "src/cmd-ed.h"
#include "src/user-out.h"
#include "src/session.h"
#include "src/screen.h"


Err ui_show_session_vi_mode(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(s) = nrows - 2;
    *session_ncols(s) = ncols;

    if (nrows <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb);
    if (!line) return "error: expecting line number, not found";
    size_t end = line + *session_nrows(s);
    end = (end > textbuf_line_count(tb)) ? textbuf_line_count(tb) : end;
    Range r = (Range){ .beg=line, .end=end };

    try( _vi_print_(tb, &r, session_conf_uout(session_conf(s))));
    return Ok;
}


Err _vi_print_(TextBuf textbuf[static 1], Range range[static 1], UserOutput out[static 1]) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        if (!line.items || !*line.items) continue; //TODO: fix get-line

        if (true) {
            if (line.len) { try( uiw_strview(out->write_std,&line)); }
            else try( uiw_lit__(out->write_std, "\n"));
        } else {
            try( ui_write_unsigned(out->write_std, linum));
            try( uiw_lit__(out->write_std,"\t"));
            if (line.len) try( uiw_strview(out->write_std, &line));
            else try( uiw_lit__(out->write_std, "\n"));
        }
    }
    return Ok;
}

static inline Err ui_vi_write_callback_stdout(const char* mem, size_t len, void* ctx) {
    (void)mem;
    (void)len;
    (void)ctx;
    return Ok;
    //if (ctx) return "error: ctx not expected in write callback";
    //return len - fwrite(mem, sizeof(const char), len, stdout) ? "error: fwrite failure": Ok;
}


static inline Err ui_vi_flush_stdout(void) {
    //if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

