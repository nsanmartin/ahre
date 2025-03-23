#include "session.h"
#include "user-input.h"
#include "utils.h"
#include "escape_codes.h"

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
        || c == '\\'
        ;
}

Err _ui_keystroke_ctrl_gg_(Session s[static 1]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    *textbuf_current_offset(tb) = 0;
    return Ok;
}

Err _ui_keystroke_ctrl_f_(Session s[static 1]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb) + *session_nrows(s);
    if (line > textbuf_line_count(tb)) line = textbuf_line_count(tb);

    size_t off;
    try(textbuf_get_offset_of_line(tb, line, &off));
    *textbuf_current_offset(tb) = off;
    return Ok;
}

Err _ui_keystroke_ctrl_b_(Session s[static 1]) {
    TextBuf* tb;
    try( session_current_buf(s, &tb));
    size_t line = textbuf_current_line(tb);
    line = (line >  *session_nrows(s)) ? line - *session_nrows(s) : 1;
    size_t off;
    try(textbuf_get_offset_of_line(tb, line, &off));
    *textbuf_current_offset(tb) = off;
    return Ok;
}


Err _ui_vi_read_vi_mode_keys_(Session s[static 1], KeyCmd cmd[static 1]) {
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
            case '\\': *cmd = (KeyCmd)c; break;
            default: continue;
        }
    }
    return Ok;
}

static inline Err _raw_readline_(char first, char* out[static 1]) {
    const size_t DEFAULT_FGETS_SIZE = 256;
    size_t len = DEFAULT_FGETS_SIZE;
    *out = std_malloc(len);
    if (!*out) return "error: realloc failure";
    *out[0] = first;
    size_t readlen = 1;

    fwrite(EscCodeSaveCursor, 1, lit_len__(EscCodeSaveCursor), stdout);
    putchar(first);
    while (1) {
        int c = fgetc(stdin);
        switch (c) {
            case KeyBackSpace: {
               if (readlen > 1) {
                   --readlen; 
                   fwrite(EscCodeEraseLine, 1, lit_len__(EscCodeEraseLine), stdout);
                   fwrite(EscCodeUnsaveCursos, 1, lit_len__(EscCodeUnsaveCursos), stdout);
                   fwrite(*out, 1, readlen, stdout);
                   continue;
               } else { (*out)[0] = '\0'; return Ok; }
            }
            case KeyCtrl_C: (*out)[0] = '\0'; return Ok;
            case KeyCtrl_D: readlen = 0; exit(0);
            default: {
                if (c != KeyEnter && !isprint(c)) continue;
                else if (readlen + 1 >= len) {
                   len += DEFAULT_FGETS_SIZE;
                   *out = std_realloc(*out, len);
                   if (!*out) return "error: realloc failure";
                } else if (c == KeyEnter) {
                   (*out)[readlen++] = '\0';
                   return Ok;
                } else {
                   (*out)[readlen++] = c;
                   putchar(c);
                   continue;
                }
            }
        }
    }
}


Err ui_vi_mode_read_input(Session* s, const char* prompt, char* out[static 1]) {
    (void)prompt;
    
    Err err = Ok;
    struct termios prev_termios;
    try( _switch_tty_to_raw_mode_(&prev_termios));

    KeyCmd cmd = keycmd_null;
    while (!cmd) { err = _ui_vi_read_vi_mode_keys_(s, &cmd); }
    if (_is_cmd_char_(cmd)) ok_then(err,_raw_readline_(cmd, out));
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    if (*out) puts("");
    return err;
}
