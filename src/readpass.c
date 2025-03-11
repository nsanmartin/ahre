#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/error.h"
#include "src/utils.h"
#include "src/user-out.h"
#include "src/user-input.h"


Err readpass_term(ArlOf(char) arl[static 1], UserOutput out[static 1]) {
    Err err = Ok;
    struct termios prev_termios;
    try( _switch_tty_to_raw_mode_(&prev_termios));

    while (1) {
        int cint = fgetc(stdin);
        if (cint == EOF) return "error: EOF reading password";
        if (cint == KeyEnter) { break; }
        if (cint == KeyCtrl_C) { arlfn(char, clean)(arl); break; }

        char c = (char)cint;
        if (!isprint(c)) continue;
        if (!arlfn(char, append)(arl, &c)) {
            err = "error: arl append failure";
            break;
        }
        try( uiw_lit__(out->write_msg,"*")); 
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    try( uiw_lit__(out->write_msg, "\n"));
    return err;
}
