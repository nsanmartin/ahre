#include "escape-codes.h"
#include "reditline.h"
#include "session.h"
#include "user-input.h"
#include "utils.h"

typedef enum {
    keycmd_null                = 0x0,
    keycmd_left_parenthesis    = '(',
    keycmd_dot                 = '.',
    keycmd_colon               = ':',
    keycmd_lt                  = '<',
    keycmd_left_square_bracket = '[',
    keycmd_left_curly_bracket  = '{',
    keycmd_pipe                =  '|',
    keycmd_inverted_bar        = '\\',
    keycmd_scroll_up,
    keycmd_scroll_down
} KeyCmd;

bool _is_cmd_char_(char c) {
    return c == ':'
        || c == '.'
        || c == '|'
        || c == '['
        || c == '{'
        || c == '('
        || c == '<'
        || c == '='
        || c == '\\'
        ;
}

Err _ui_keystroke_ctrl_gg_(Session s[_1_]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    *textbuf_current_offset(tb) = 0;
    return Ok;
}

Err _ui_keystroke_ctrl_f_(Session s[_1_]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb) + *session_nrows(s);
    if (line > textbuf_line_count(tb)) line = textbuf_line_count(tb);

    size_t off;
    try(textbuf_get_offset_of_line(tb, line, &off));
    *textbuf_current_offset(tb) = off;
    return Ok;
}

Err _ui_keystroke_ctrl_b_(Session s[_1_]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb);
    line = (line >  *session_nrows(s)) ? line - *session_nrows(s) : 1;
    size_t off;
    try(textbuf_get_offset_of_line(tb, line, &off));
    *textbuf_current_offset(tb) = off;
    return Ok;
}


Err _ui_vi_read_vi_mode_keys_(Session s[_1_], KeyCmd cmd[_1_]) {
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
            case ':':
            case '.':
            case '|':
            case '[':
            case '{':
            case '(':
            case '<':
            case '=':
            case '\\': *cmd = (KeyCmd)c; break;
            default: continue;
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
        while (!cmd) { err = _ui_vi_read_vi_mode_keys_(s, &cmd); }
        if (_is_cmd_char_(cmd)) ok_then(err,_raw_reditline_(cmd, session_input_history(s), out));
        else break;
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    if (*out) puts("");
    return err;
}
