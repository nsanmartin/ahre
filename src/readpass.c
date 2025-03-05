#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "src/error.h"
#include "src/utils.h"
#include "src/user-out.h"

enum KEY_ACTION{
	KEY_NULL = 0,	    /* NULL */
	CTRL_A = 1,         /* Ctrl+a */
	CTRL_B = 2,         /* Ctrl-b */
	CTRL_C = 3,         /* Ctrl-c */
	CTRL_D = 4,         /* Ctrl-d */
	CTRL_E = 5,         /* Ctrl-e */
	CTRL_F = 6,         /* Ctrl-f */
	CTRL_H = 8,         /* Ctrl-h */
	TAB = 9,            /* Tab */
	CTRL_K = 11,        /* Ctrl+k */
	CTRL_L = 12,        /* Ctrl+l */
	ENTER = 13,         /* Enter */
	CTRL_N = 14,        /* Ctrl-n */
	CTRL_P = 16,        /* Ctrl-p */
	CTRL_T = 20,        /* Ctrl-t */
	CTRL_U = 21,        /* Ctrl+u */
	CTRL_W = 23,        /* Ctrl+w */
	ESC = 27,           /* Escape */
	BACKSPACE =  127    /* Backspace */
};


static Err _switch_tty_to_raw_mode_(struct termios prev_termios[static 1]) {

    if (!isatty(STDIN_FILENO)) return "error: not a tty";
    if (tcgetattr(STDIN_FILENO, prev_termios) == -1) return "error: tcgetattr falure";

    struct termios new_termios = *prev_termios;
    new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    new_termios.c_oflag &= ~(OPOST);
    new_termios.c_cflag |= (CS8);
    new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO,TCSAFLUSH, &new_termios) < 0) return "error: tcsetattr failure";
    return Ok;
}

typedef enum { 
    Ctrl_c   = 3,
    KeyEnter = 13
} KeyStroke;

Err readpass_term(ArlOf(char) arl[static 1], WriteUserOutputCallback write) {
    Err err = Ok;
    struct termios prev_termios;
    try( _switch_tty_to_raw_mode_(&prev_termios));

    while (1) {
        int cint = fgetc(stdin);
        if (cint == EOF) return "error: EOF reading password";
        if (cint == KeyEnter) { break; }
        if (cint == Ctrl_c) { arlfn(char, clean)(arl); break; }

        char c = (char)cint;
        if (!isprint(c)) continue;
        if (!arlfn(char, append)(arl, &c)) {
            err = "error: arl append failure";
            break;
        }
        try( uiw_lit__(write,"*")); 
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &prev_termios) == -1) return "error: tcsetattr failure";
    fwrite("\n", 1, 1, stdout);
    return err;
}
