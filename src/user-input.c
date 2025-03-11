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
        if (cint == KeyCtrl_C) { continue; }

        char c = (char)cint;
        if (!isprint(c)) continue;
        if (_is_cmd_char_(c)) { *cmd = c; }
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
        err = _ui_vi_read_vi_mode_keys_(s, &cmd, out);
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    ok_then(err,_raw_readline_(cmd, out));
    return err;
}
