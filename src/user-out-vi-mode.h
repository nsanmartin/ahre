#ifndef AHRE_USER_OUT_VI_MODE_H__
#define AHRE_USER_OUT_VI_MODE_H__

#include "src/textbuf.h"
#include "src/ranges.h"
#include "src/user-out.h"
#include "src/user-interface.h"

Err _vi_print_(TextBuf textbuf[static 1], Range range[static 1], UserOutput out[static 1]);


static Err ui_show_session_vi_mode(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(s) = nrows;
    *session_ncols(s) = ncols;

    if (nrows <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = *screen_line(textbuf_screen(tb));
    if (!line) *screen_line(textbuf_screen(tb)) = line = 1;
    Range r = (Range){ .beg=line, .end=line + nrows };

    try( _vi_print_(tb, &r, session_conf_uout(session_conf(s))));
    return Ok;
}


static inline UserOutput uout_vi_mode(void) {
    return (UserOutput) {
        .write_msg    = ui_write_callback_stdout,
        .flush_msg    = ui_flush_stdout,
        .write_std    = ui_write_callback_stdout,
        .flush_std    = ui_flush_stdout,
        .show_session = ui_show_session_vi_mode
    };
}

#endif
