#include "user-out.h"
#include "session.h"

#include "generic.h"


static Err ui_vi_flush_msg(Session* s, CmdOut cout[_1_]);
static Err ui_vi_flush_std(Session* s, CmdOut cout[_1_]);


static Err
flush_to_file_msg(Msg msg[_1_], FilePtr fp) {
    if (!msg_len(msg)) return Ok;
    try( mem_fwrite(msg_items(msg), msg_len(msg), fp.ptr));
    if (fflush(fp.ptr)) return err_fmt("error: fflush failure: %s", strerror(errno));
    msg_reset(msg);
    return Ok;
}

static Err
flush_to_file_str(Str s[_1_], FilePtr fp) {
    if (!len__(s)) return Ok;
    try( mem_fwrite(items__(s), len__(s), fp.ptr));
    if (fflush(fp.ptr)) return err_fmt("error: fflush failure: %s", strerror(errno));
    str_reset(s);
    return Ok;
}


static Err
flush_to_file_strview(StrView s, FilePtr fp) {
    if (!s.len) return Ok;
    try( mem_fwrite(s.items, s.len, fp.ptr));
    if (fflush(fp.ptr)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

#define flush_to_file(S, F) _Generic((S),\
    Str*: flush_to_file_str,\
    Msg*: flush_to_file_msg\
)(S,F)

/*
 * Line mode
 */


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


static Err
ui_line_flush_std_to_file(CmdOut cout[_1_]) {
    return cmd_out_sink_to_file_flush(&cout->std_sink, cmd_out_std(cout));
}

static Err
ui_line_flush_std_to_shell(CmdOut cout[_1_]) {
    (void)cout;
    try( flush_to_file_strview(svl( "not implemented"), (FilePtr){.ptr=stdout}));
    return Ok;
}

static Err
ui_line_flush_msg_to_file(CmdOut cout[_1_]) {
    return cmd_out_sink_to_file_flush(&cout->msg_sink, msg_str(cmd_out_msg(cout)));
}

static Err
ui_line_flush_msg_to_shell(CmdOut cout[_1_]) {
    (void)cout;
    try( flush_to_file_strview(svl( "not implemented"), (FilePtr){.ptr=stdout}));
    return Ok;
}

Err
ui_line_flush_std(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: no session";
    Err err = Ok;
    if  (cmd_out_has_std(cout)) {
        switch (cout->std_sink.tag) {
            case CMD_OUT_SCREEN:  err = flush_to_file(cmd_out_std(cout), (FilePtr){.ptr=stdout});
                break;
            case CMD_OUT_WARN:  err = flush_to_file(cmd_out_std(cout), (FilePtr){.ptr=stderr});
                break;
            case CMD_OUT_FILE: err = ui_line_flush_std_to_file(cout);
                break;
            case CMD_OUT_SHELL: err = ui_line_flush_std_to_shell(cout);
                break;
            default: fail_e("invalid output redirection");
        }
    }
    try(cmd_out_sink_cleanup(&cout->std_sink, CMD_OUT_SCREEN));
    return err;
}


Err
ui_line_flush_msg(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: no session";

    Err err = Ok;
    if  (cmd_out_has_msg(cout)) {
        switch (cout->msg_sink.tag) {
            case CMD_OUT_SCREEN:  err = flush_to_file(cmd_out_msg(cout), (FilePtr){.ptr=stdout});
                break;
            case CMD_OUT_WARN:  err = flush_to_file(cmd_out_msg(cout), (FilePtr){.ptr=stderr});
                break;
            case CMD_OUT_FILE: err = ui_line_flush_msg_to_file(cout);
                break;
            case CMD_OUT_SHELL: err = ui_line_flush_msg_to_shell(cout);
                break;
            default: fail_e("invalid output redirection");
        }
    }
    try(cmd_out_sink_cleanup(&cout->msg_sink, CMD_OUT_WARN));
    return err;
}

Err ui_line_show_session(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: unexpected null session, this should really not happen";
    try (ui_line_flush_msg(s, cout));
    try (ui_line_flush_std(s, cout));
    return Ok;
}
/*
 * Visual mode
 */


#define _vi_write_std_to_screen_lit__(Ses, Lit) _vi_write_std_to_screen_(Ses, Lit, lit_len__(Lit))


static Err _vi_print_range_std_mod_(
    TextBuf textbuf[_1_], Range range[_1_], Session* s, CmdOut cout[_1_]
) {
    return session_write_screen_range_mod(s, textbuf, range, cout);
}


static void _update_if_smaller_(size_t value[_1_], size_t new_value) {
    if (*value > new_value) *value = new_value;
}

static Err
show_session_get_show_range(Session* s, TextBuf tb[_1_], Range out[_1_]) {
    const size_t win_rows = *session_nrows(s);
    const size_t buf_rows = textbuf_line_count(tb);
    if (buf_rows <= win_rows) {
        *out = (Range){ .beg=1, .end=buf_rows};
        return Ok;
    }

    size_t line;
    try( textbuf_get_current_line_number(tb,&line));
    if (!line) return "error: expecting current line number, not found";

    const size_t remaining = buf_rows - line;
    if (remaining >= win_rows)
        *out = (Range){ .beg=line, .end=line + win_rows };
    else {
        const size_t linenum = buf_rows - win_rows;
        try( textbuf_get_offset_of_line(tb, linenum, textbuf_current_offset(tb)));
        *out = (Range){ .beg=linenum, .end=buf_rows};
    }

    return Ok;
}

#define EMPTY_SESSION_MSG_ "Session is empty\n\nType '\\?' (slash-question mark) for help.\n"
#define EMPTY_BUFFER_MSG_ "Buffer is empty\n"
#define BUFFER_NOT_PARSED_MSG_ "content was not parsed, is it html?\ncheck source and run .parse\n"
/*
 * The "default" session is the onw shown when there is no "screen" as a
 * result of the user cmd.
 */
static Err
ui_vi_show_session_default(Session* s, CmdOut cout[_1_]) {
    Err err             = Ok;

    if (session_is_empty(s)) {
        try( screen__(cout, svl(EMPTY_SESSION_MSG_)));
        return Ok;
    } else {
        HtmlDoc* d;
        TextBuf* tb;
        try( session_current_doc(s, &d));
        try( session_current_buf(s, &tb));

        if (textbuf_is_empty(tb)) {
            if (!htmldoc_content_is_html(d)) 
                try( msg_ln__(cout, svl(BUFFER_NOT_PARSED_MSG_)));
            else try( msg__(cout, svl(EMPTY_BUFFER_MSG_)));
        } else {

            Range r;
            try(show_session_get_show_range(s, tb, &r));

            err = _vi_print_range_std_mod_(tb, &r, s, cout);
        }
    }
    return err;

}

Err ui_vi_show_session(Session* s, CmdOut cout[_1_]) {

    if (!s) return "error: unexpected null session, this should really not happen";

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    _update_if_smaller_(session_nrows(s), nrows - 2);
    _update_if_smaller_(session_ncols(s), ncols);
    if (nrows <= 3) return "too few rows in window to show session";

    try( lit_write__(EscCodeClsScr, stdout));
    bool screen_touched = cmd_out_will_touch_screen(cout);



    if (cout->msg_sink.tag == CMD_OUT_FILE && cout->std_sink.tag == CMD_OUT_FILE
        && str_eq(cout->msg_sink.filename, cout->std_sink.filename)
        && (cmd_out_has_std(cout) || cmd_out_has_msg(cout))
    ) {

        Err err    = Ok;
        FilePtr fp = (FilePtr){0};
        try(file_open(cout->std_sink.filename.items, "w", &fp.ptr));

        tryjmp(err,Clean,flush_to_file(cmd_out_msg(cout), fp));
        tryjmp(err,Clean,flush_to_file(cmd_out_std(cout), fp));
    Clean:
        file_close(fp.ptr);
        if (err) return err;
        try(cmd_out_sink_cleanup(&cout->std_sink, CMD_OUT_SCREEN));
        try(cmd_out_sink_cleanup(&cout->msg_sink, CMD_OUT_WARN));

    } else {
        try( ui_vi_flush_msg(s, cout));
        try( ui_vi_flush_std(s, cout));
    }


    if (!screen_touched) {
        try( ui_vi_show_session_default(s, cout));
        try( ui_vi_flush_msg(s, cout));
        try( ui_vi_flush_std(s, cout));
    }


    return Ok;
}


static Err
ui_vi_flush_std_to_msg(Session s[_1_], CmdOut cout[_1_]) {
    Str* std = cmd_out_std(cout);
    try( ui_vi_flush_msg_read_input(s, sv(std)));
    str_reset(std);
    return lit_write__(EscCodeClsScr, stdout);
}

static Err
ui_vi_flush_std_to_file(CmdOut cout[_1_]) {
    return cmd_out_sink_to_file_flush(&cout->std_sink, cmd_out_std(cout));
}

static Err
ui_vi_flush_std_to_shell(Session s[_1_], CmdOut cout[_1_]) {
    (void)cout;
    try( ui_vi_flush_msg_read_input(s, svl("flush std to shell not implemented")));
    try( lit_write__(EscCodeClsScr, stdout));
    return Ok;
}

static Err
ui_vi_flush_std(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: no session";
    Err err = Ok;
    if  (cmd_out_has_std(cout)) {
        switch (cout->std_sink.tag) {
            case CMD_OUT_SCREEN:  err = flush_to_file(cmd_out_std(cout), (FilePtr){.ptr=stdout});
                break;
            case CMD_OUT_WARN:  err = ui_vi_flush_std_to_msg(s, cout);
                break;
            case CMD_OUT_FILE: err = ui_vi_flush_std_to_file(cout);
                break;
            case CMD_OUT_SHELL: err = ui_vi_flush_std_to_shell(s, cout);
                break;
            default: fail_e("invalid output redirection");
        }
    }
    try(cmd_out_sink_cleanup(&cout->std_sink, CMD_OUT_SCREEN));
    return err;
}


static Err
ui_vi_flush_msg_default(Session s[_1_], CmdOut cout[_1_]) {
    Msg* msg = cmd_out_msg(cout);
    try( ui_vi_flush_msg_read_input(s, sv(msg->s)));
    msg_reset(msg);
    return lit_write__(EscCodeClsScr, stdout);
}

static Err
ui_vi_flush_msg_to_std(CmdOut cout[_1_]) {
    return flush_to_file(cmd_out_msg(cout), (FilePtr){.ptr=stdout});
}

static Err
ui_vi_flush_msg_to_file(CmdOut cout[_1_]) {
    return cmd_out_sink_to_file_flush(&cout->msg_sink, msg_str(cmd_out_msg(cout)));
}

static Err
ui_vi_flush_msg_to_shell(Session s[_1_], CmdOut cout[_1_]) {
    (void)cout;
    try( ui_vi_flush_msg_read_input(s, svl("flush msg to shell not implemented")));
    try( lit_write__(EscCodeClsScr, stdout));
    return Ok;
}

static Err
ui_vi_flush_msg(Session* s, CmdOut cout[_1_]) {
    if (!s) return "error: no session";
    Err err = Ok;
    if  (cmd_out_has_msg(cout)) {
        switch (cout->msg_sink.tag) {
            case CMD_OUT_WARN:    err = ui_vi_flush_msg_default(s, cout); break;
            case CMD_OUT_SCREEN:  err = ui_vi_flush_msg_to_std(cout); break;
            case CMD_OUT_FILE:    err = ui_vi_flush_msg_to_file(cout); break;
            case CMD_OUT_SHELL:   err = ui_vi_flush_msg_to_shell(s, cout); break;
            default: fail_e("invalid output redirection");
        }
    }
    try(cmd_out_sink_cleanup(&cout->msg_sink, CMD_OUT_WARN));
    return err;
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

