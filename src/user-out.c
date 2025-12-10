#include "user-out.h"
#include "session.h"

/*
 * Line mode
 */

Err ui_line_show_session(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";
    if (session_is_empty(s)) return Ok;
    return session_uout(s)->flush_std(NULL);
}

Err ui_line_show_err(Session* s, char* err, size_t len) {
    (void)s;
    if (err) {
        if ((len != fwrite(err, 1, len, stderr))
        || (1 != fwrite("\n", 1, 1, stderr))
        ) return "error: fwrite failure while attempting to show an error :/";
    }
    fflush(stderr);
    return Ok;
}

/*
 * Visual mode
 */

#include "ranges.h"
#include "session.h"
#include "textbuf.h"
#include "escape-codes.h"


#define _vi_write_std_to_screen_lit__(Ses, Lit) _vi_write_std_to_screen_(Ses, Lit, lit_len__(Lit))

// static inline Err _vi_write_unsigned_std_screen_(Session s[_1_], uintmax_t ui) {
//     return ui_write_unsigned(session_conf_uout(session_conf(s))->write_std, ui, s);
// }

Err ui_vi_write_std(const char* mem, size_t len, Session* s) {
    if (!s) return "error: no session";
    HtmlDoc* doc;
    try( session_current_doc(s, &doc));
    Str* screen = htmldoc_screen(doc);
    return str_append(screen, (char*)mem, len);
}


static Err _vi_print_range_std_mod_(TextBuf textbuf[_1_], Range range[_1_], Session* s) {
    return session_write_std_range_mod(s, textbuf, range);
}


static void _update_if_smaller_(size_t value[_1_], size_t new_value) {
    if (*value > new_value) *value = new_value;
}

#define EMPTY_SESSION_MSG_ "Session is empty\n"
#define EMPTY_BUFFER_MSG_ "Buffer is empty\n"
Err ui_vi_show_session(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";
    if (session_is_empty(s)) {
        try( msg_append_lit__(session_msg(s), EMPTY_SESSION_MSG_));
        session_uout(s)->flush_std(s);
        return Ok;
    }


    TextBuf* tb;
    try( session_current_buf(s, &tb));
    if (textbuf_is_empty(tb)) {
        try( msg_append_lit__(session_msg(s), EMPTY_BUFFER_MSG_));
        session_uout(s)->flush_std(s);
        return Ok;
    }

    session_uout(s)->flush_msg(s);
    try( lit_write__(EscCodeClsScr, stdout));
    size_t line = textbuf_current_line(tb);
    if (!line) return "error: expecting current line number, not found";

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    _update_if_smaller_(session_nrows(s), nrows - 2);
    _update_if_smaller_(session_ncols(s), ncols);

    if (nrows <= 3) return "too few rows in window to show session";
    size_t end = line + *session_nrows(s);
    end = (end > textbuf_line_count(tb)) ? textbuf_line_count(tb) : end;
    Range r = (Range){ .beg=line, .end=end };

    try( _vi_print_range_std_mod_(tb, &r, s));
    session_uout(s)->flush_std(s);
    return Ok;
}

Err ui_vi_flush_std(Session* s) {
    if (!s) return "error: no session";
    Msg* msg = session_msg(s);
    if (msg_len(msg)) {
        try( mem_fwrite(msg_items(msg), msg_len(msg), stdout));
        if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
        msg_reset(msg);
    }
    if (!session_is_empty(s)) {
        HtmlDoc* doc;
        try( session_current_doc(s, &doc));
        Str* screen = htmldoc_screen(doc);
        if (!len__(screen)) return Ok;
        try( mem_fwrite(items__(screen), len__(screen), stdout));
        if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
        str_reset(screen);
    }
    return Ok;
}


Err ui_vi_write_msg(const char* mem, size_t len, Session* s) {
    if (!s) return "error: no session";
    return msg_append(session_msg(s), (char*)mem, len);
}

#define MSG_PREFIX_ "{msg}:\n"
#define CONTINUE_MSG_ "{% type enter to continue %}"
Err ui_vi_flush_msg(Session* s) {
    if (!s) return "error: no session";
    Msg* msg = session_msg(s);
    if (!msg_len(msg)) return Ok;
    try( lit_write__(MSG_PREFIX_, stdout));
    try( mem_fwrite(msg_items(msg), msg_len(msg), stdout));
    try( lit_write__(CONTINUE_MSG_, stdout));
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    getchar();
    msg_reset(msg);
    return Ok;
}

Err ui_vi_show_err(Session* s, char* err, size_t len) {
    (void)s;
    FILE* stream = stdout;
    if (err) {
        if (mem_fwrite(err, len, stream)
        || lit_write__("{type enter}", stream)) {
            return "error: fprintf failure while attempting to show an error :/";
        }

    }
    fflush(stream);
    getchar();
    return Ok;
}

