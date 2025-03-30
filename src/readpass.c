#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "utils.h"
#include "user-out.h"
#include "user-input.h"
#include "session.h"

Err readpass_term(ArlOf(char) passw[static 1], SessionMemWriter w[static 1]) {
    Err err = Ok;
    struct termios prev_termios;
    try( _switch_tty_to_raw_mode_(&prev_termios));

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
        try( w->write(w, "*", 1));
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    try( w->write(w, "\n", 1));
    return err;
}
