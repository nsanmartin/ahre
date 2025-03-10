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

Err
_ui_vi_read_vi_mode_keys_(Session s[static 1], char cmd[static 1], char* line[static 1]) {
    (void)s;
    (void)line;
    while (!*cmd) {
        int cint = fgetc(stdin);
        if (cint == EOF) return "error: EOF reading input";
        if (cint == KeyEnter) { continue; }
        if (cint == Ctrl_c) { continue; }

        char c = (char)cint;
        if (!isprint(c)) continue;
        if (_is_cmd_char_(c)) { *cmd = c; }
    }
    return Ok;
}

Err ui_vi_mode_read_input(Session* s, const char* prompt, char* out[static 1]) {
    (void)s;
    (void)prompt;
    
    Err err = Ok;
    struct termios prev_termios;
    try( _switch_tty_to_raw_mode_(&prev_termios));

    char cmd = 0x0;
    while (!cmd) {
        err = _ui_vi_read_vi_mode_keys_(s, &cmd, out);
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    //TODO: get this fn from UserInput
    char prompt_char[2] = { cmd, '\0' };
    //ok_then(err,ui_isocline_readline(s, prompt_char, out));
    ok_then(err,ui_fgets_readline(s, prompt_char, out));
    return err;
}
