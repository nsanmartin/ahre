#include "src/session.h"
#include "src/user-input.h"
#include "src/utils.h"

//typedef enum { _keep_reading_, _read_line_ } ReadKeyViModeState;

bool _is_cmd_char_(char c) {
    return c == ':'
        || c == '|'
        || c == '['
        || c == '{'
        || c == '('
        || c == '\\'
        ;
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

Err _ui_vi_read_vi_mode_keys_(Session s[static 1], char cmd[static 1]) {
    while (!*cmd) {
        int cint = fgetc(stdin);
        char c;
        switch(cint) {
            case EOF: return "error: EOF reading input";
            case KeyEnter: 
            case KeyCtrl_C: continue;
            case KeyCtrl_F:
            case KeySpace: _ui_keystroke_ctrl_f_(s); *cmd = KeyCtrl_F; break; 
            case KeyBackSpace:
            case KeyCtrl_B: _ui_keystroke_ctrl_b_(s); *cmd = KeyCtrl_B; break; 

            default:

            c = (char)cint;
            if (!isprint(c)) continue;
            if (_is_cmd_char_(c)) { *cmd = c; }
        }
    }
    return Ok;
}

static inline Err _raw_readline_(char first, char* out[static 1]) {
    const size_t DEFAULT_FGETS_SIZE = 256;
    size_t len = DEFAULT_FGETS_SIZE;
    size_t readlen = 0;
    *out = std_malloc(len);
    if (!*out) return "error: realloc failure";
    if (first != '\\') {
        *out[0] = first;
        ++readlen;
    } 

    putchar(first);
    while (1) {
        char* line = fgets(*out + readlen, len - readlen, stdin);
        if (!line) {
            if (feof(stdin)) { clearerr(stdin); *out[0] = '\0'; return Ok; }
            return err_fmt("error: fgets failure: %s", strerror(errno));
        }
        if (strchr(line, '\n')) return Ok;
        readlen = len - 1;
        len += DEFAULT_FGETS_SIZE;
        *out = std_realloc(*out, len);
        if (!*out) return "error: realloc failure";
    }
}


Err ui_vi_mode_read_input(Session* s, const char* prompt, char* out[static 1]) {
    (void)prompt;
    
    Err err = Ok;
    struct termios prev_termios;
    try( _switch_tty_to_raw_mode_(&prev_termios));

    char cmd = 0x0;
    while (!cmd) {
        err = _ui_vi_read_vi_mode_keys_(s, &cmd);
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    if (_is_cmd_char_(cmd)) ok_then(err,_raw_readline_(cmd, out));
    return err;
}
