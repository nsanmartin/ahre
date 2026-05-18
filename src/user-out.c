#include "user-out.h"
#include "session.h"

#include "generic.h"
/*
 * Line mode
 */

Err ui_line_show_session(Session* s, CmdOut cout[_1_]) {
    (void)cout;
    if (!s) return "error: unexpected null session, this should really not happen";
    return session_flush_cmd_out(s, cout);
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


static Err _vi_print_range_std_mod_(
    TextBuf textbuf[_1_], Range range[_1_], Session* s, CmdOut cout[_1_]
) {
    return session_write_screen_range_mod(s, textbuf, range, cout);
}


static void _update_if_smaller_(size_t value[_1_], size_t new_value) {
    if (*value > new_value) *value = new_value;
}

#define EMPTY_SESSION_MSG_ "Session is empty\n\nType '\\?' (slash-question mark) for help.\n"
#define EMPTY_BUFFER_MSG_ "Buffer is empty\n"
/*
 * The "default" session is the onw shown when there is no "screen" as a
 * result of the user cmd.
 */
static Err ui_vi_show_session_default(Session* s) {
    CmdOut* default_out = &(CmdOut){0};
    Err err             = Ok;

    if (session_is_empty(s)) {
        try( screen__(default_out, svl(EMPTY_SESSION_MSG_)));
        err = session_flush_cmd_out_screen(s, default_out);
    } else {
        HtmlDoc* d;
        TextBuf* tb;
        try( session_current_doc(s, &d));
        try( session_current_buf(s, &tb));
        if (textbuf_is_empty(tb)) {
            if (!htmldoc_content_is_html(d)) 
                try( msg_ln__(default_out, svl("content was not parsed, is it html?\ncheck source and run .parse")));
            else try( msg__(default_out, svl(EMPTY_BUFFER_MSG_)));
            err = session_flush_cmd_out_msg(s, default_out);
        } else {


            size_t line = textbuf_current_line(tb);
            if (!line) return "error: expecting current line number, not found";

            size_t end = line + *session_nrows(s);
            end = (end > textbuf_line_count(tb)) ? textbuf_line_count(tb) : end;
            Range r = (Range){ .beg=line, .end=end };

            err = _vi_print_range_std_mod_(tb, &r, s, default_out);
            ok_then(err, session_flush_cmd_out_screen(s, default_out));
        }
    }
    cmd_out_clean(default_out);
    return err;

}

Err ui_vi_show_session(Session* s, CmdOut cout[_1_]) {

    if (!s) return "error: unexpected null session, this should really not happen";

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    _update_if_smaller_(session_nrows(s), nrows - 2);
    _update_if_smaller_(session_ncols(s), ncols);
    if (nrows <= 3) return "too few rows in window to show session";

    //TODO: instead stdout pass CmdOut
    try( lit_write__(EscCodeClsScr, stdout));
    try( session_flush_cmd_out_msg(s, cout));

    if (len__(cmd_out_screen(cout))) return session_flush_cmd_out_screen(s, cout);
    return ui_vi_show_session_default(s);
}


Err ui_vi_flush_std(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: no session";
    Str* screen = cmd_out_screen(cout);
    if (len__(screen)) {
        try( mem_fwrite(items__(screen), len__(screen), stdout));
        if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
        str_reset(screen);
    }
    return Ok;
}


Err ui_vi_flush_msg(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: no session";
    Msg* msg = cmd_out_msg(cout);
    if (!msg_len(msg)) return Ok;

    try(ui_vi_flush_msg_read_input(s, sv(msg->s)));
    msg_reset(msg);
    return lit_write__(EscCodeClsScr, stdout);
}

Err ui_vi_show_err(Session* s, char* err, size_t len) {
    (void)s;
    FILE* stream = stdout;
    if (err) {
        if (internal_error(err)) {
            int suberr = mem_fwrite(EscCodeRed, lit_len__(EscCodeRed), stream)
                      || mem_fwrite(INTERNAL_ERROR_PREFIX, lit_len__(INTERNAL_ERROR_PREFIX), stream)
                      || mem_fwrite(EscCodeReset, lit_len__(EscCodeRed), stream)
                      || mem_fwrite(err + lit_len__(INTERNAL_ERROR_PREFIX), len - lit_len__(INTERNAL_ERROR_PREFIX), stream)
                      || lit_write__("{type q}", stream)
                      ;
            if (suberr) return EscCodeRed"error: fprintf failure while attempting to show an error :/"EscCodeReset;
        } else if (mem_fwrite(err, len, stream) || lit_write__("{type q}", stream)) {
            return "error: fprintf failure while attempting to show an error :/";
        }

    }
    fflush(stream);
    try( wait_for_char('q'));
    return lit_write__(EscCodeClsScr, stream);
}

