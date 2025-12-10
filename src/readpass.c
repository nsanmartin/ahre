#include "reditline.h"

Err readpass_term(ArlOf(char) passw[_1_], bool write) {
    Err err = Ok;
    struct termios prev_termios;
    try( switch_tty_to_raw_mode(&prev_termios));

    while (1) {
        int cint = fgetc(stdin);
        if (cint == EOF) return "error: EOF reading password";
        if (cint == KeyEnter) { break; }
        if (cint == KeyCtrl_C) { arlfn(char, clean)(passw); break; }

        char c = (char)cint;
        if (!isprint(c)) continue;
        if (!arlfn(char, append)(passw, &c)) {
            err = "error: passw append failure";
            break;
        }
        if (write) fwrite("*", 1, 1, stdout);
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    if (write) fwrite("\n", 1, 1, stdout);
    return err;
}
