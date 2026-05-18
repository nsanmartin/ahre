#include "escape-codes.h"
#include "reditline.h"
#include "session.h"
#include "user-input.h"
#include "utils.h"
#include "generic.h"


typedef enum {
    keycmd_null                = 0x0,
    keycmd_dolar_sign          = '$',
    keycmd_left_parenthesis    = '(',
    keycmd_dot                 = '.',
    keycmd_colon               = ':',
    keycmd_lt                  = '<',
    keycmd_eq                  = '=',
    keycmd_left_square_bracket = '[',
    keycmd_left_curly_bracket  = '{',
    keycmd_pipe                = '|',
    keycmd_inverted_bar        = '\\',
    keycmd_scroll_up,
    keycmd_scroll_down
} KeyCmd;

static bool _is_cmd_char_(char c) {
    return c == keycmd_dolar_sign
        || c == keycmd_left_parenthesis
        || c == keycmd_dot
        || c == keycmd_colon
        || c == keycmd_lt
        || c == keycmd_eq
        || c == keycmd_left_square_bracket
        || c == keycmd_left_curly_bracket
        || c == keycmd_pipe
        || c == keycmd_inverted_bar
        ;
}


static Err _ui_keystroke_ctrl_f_(Session s[_1_]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb) + *session_nrows(s);
    if (line > textbuf_line_count(tb)) line = textbuf_line_count(tb);

    size_t off;
    try(textbuf_get_offset_of_line(tb, line, &off));
    *textbuf_current_offset(tb) = off;
    return Ok;
}

static Err _ui_keystroke_ctrl_b_(Session s[_1_]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb);
    line = (line >  *session_nrows(s)) ? line - *session_nrows(s) : 1;
    size_t off;
    try(textbuf_get_offset_of_line(tb, line, &off));
    *textbuf_current_offset(tb) = off;
    return Ok;
}


static Err _ui_vi_read_vi_mode_keys_(Session s[_1_], KeyCmd cmd[_1_]) {
    while (!*cmd) {
        int c = fgetc(stdin);
        switch(c) {
            case EOF: return "error: EOF reading input";
            case KeyEnter: 
            case KeyCtrl_C: continue;
            case KeyCtrl_F:
            case KeySpace: _ui_keystroke_ctrl_f_(s); *cmd = keycmd_scroll_up; break; 
            case KeyBackSpace:
            case KeyCtrl_B: _ui_keystroke_ctrl_b_(s); *cmd = keycmd_scroll_down; break; 
            default:
                if (_is_cmd_char_(c)) {
                    *cmd = (KeyCmd)c; break;
                } else continue;
        }
    }
    return Ok;
}

static inline Err _raw_reditline_(char first, ArlOf(const_cstr) history[_1_], char* out[_1_]) {
    char buf[] = { first, '\0' };
    *out = reditline(NULL, buf, history);
    if (reditline_error(*out)) return *out;
    if (*out && redit_history_add(history, *out)) return "error: reditline_history failure";
    return Ok;
}


Err ui_vi_mode_read_input(Session* s, const char* prompt, char* out[_1_]) {
    (void)prompt;
    
    Err err = Ok;
    struct termios prev_termios;
    try( switch_tty_to_raw_mode(&prev_termios));

    *out = NULL;

    while (!*out) {
        KeyCmd cmd = keycmd_null;
        while (!err && !cmd) { err = _ui_vi_read_vi_mode_keys_(s, &cmd); }
        if (!err && _is_cmd_char_(cmd)) ok_then(err,_raw_reditline_(cmd, session_input_history(s), out));
        else break;
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    if (*out) puts("");
    return err;
}

static StrView
_get_next_page_(size_t nrows, size_t ncols, StrView msg[_1_]) {
    const char*  beg       = msg->items;
    size_t       len = 0;

    while (msg->len > 0 && nrows) {
        StrView      line            = strview_split_line(msg);
        const size_t available_space = nrows * ncols;
        const size_t rows_in_line    = 1 + (line.len / ncols) - (bool)(line.len && line.len % ncols == 0);
        if (line.len <= available_space) {
            nrows -= rows_in_line;
            len   += line.len + 1;
        } else if (rows_in_line >= nrows) {
            const size_t rewind = line.len - available_space;
            if (beg + rewind > msg->items) {
                msg->len = 0;
                return svl("\nerror paginating msg, rewind too large! :(\n");
            }
            nrows       = 0;
            len        += available_space;
            msg->len   += rewind;
            msg->items -= rewind;
        } else {
            msg->len = 0;
            return svl("\nerror paginating msg :(\n");
        }
    }
        return sv(beg, len);
}


#define MSG_PREFIX_ "{msg}\n"
static StrView
_continue_msg_(size_t pages_len, size_t pagenum, size_t msg_len) {
    if (pages_len == 1 && !msg_len) return svl("{type q}");
    if (!msg_len && pagenum + 1 == pages_len) return svl("{type backspace or q}");
    if (!pagenum) return svl("{type space or q}");
    return svl("{type space, backspace or q}");
}

Err
ui_vi_flush_msg_read_input(Session* s, StrView msg) {
    if (!msg.len) return Ok;
    Err e = Ok;

    bool           loop    = true;
    bool           update  = true;
    size_t         pagenum = 0;
    ArlOf(StrView) ps      = (ArlOf(StrView)){0};
    StrView*       pp      = NULL;

    for (;loop;) {
        if (ps.len == pagenum) {
            StrView page = _get_next_page_(*session_nrows(s) - 1, *session_ncols(s), &msg);
            pp = arlfn(StrView,append)(&ps, &page);
        } else if (ps.len > pagenum) {
            pp = arlfn(StrView,at)(&ps, pagenum);
        } else {e=err_internal("invalid msg page"); goto Clean;}

        if (update) {
            tryjmp(e,Clean, lit_write__(EscCodeClsScr, stdout));
            tryjmp(e,Clean, lit_write__(MSG_PREFIX_, stdout));
            tryjmp(e,Clean, mem_fwrite(pp->items, pp->len, stdout));
            StrView continue_msg = _continue_msg_(ps.len, pagenum, msg.len);
            tryjmp(e,Clean, mem_fwrite(continue_msg.items, continue_msg.len,  stdout));
            if (fflush(stdout)) {e=err_fmt("error: fflush failure: %s", strerror(errno)); goto Clean;}
        }

        struct termios prev_termios;
        try( switch_tty_to_raw_mode(&prev_termios));

        int c = fgetc(stdin);
        switch(c) {
            case EOF:
            case KeyCtrl_C:
            case 'q':
                loop = false;
                break;
            case KeyCtrl_F:
            case KeySpace:
               if ((pagenum < ps.len && msg.len) || (pagenum + 1 < ps.len)) {
                   update = true;
                   ++pagenum;
               } else update = false;
                break;
            case KeyBackSpace:
            case KeyCtrl_B:
               if (pagenum) {
                   --pagenum;
                   update = true;
               } else update = false;
               break;
            default:
               update = false;
               break;
        }
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return err_internal("tcsetattr failure");
    }
Clean:
    arlfn(StrView,clean)(&ps);
    return e;
}

Err wait_for_char(char c) {
    struct termios prev_termios;
    try( switch_tty_to_raw_mode(&prev_termios));
    int cin;
    while ((c != (cin = fgetc(stdin))))
        ;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return err_internal("tcsetattr failure");
    return Ok;
}
