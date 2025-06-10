#ifndef AHRE_REDITLINE_H__
#define AHRE_REDITLINE_H__

#include <termios.h>

#include "str.h"

void reditline_history_cleanup(void) ;

typedef enum {
	KeyNull      = 0,
	KeyCtrl_A    = 1,
	KeyCtrl_B    = 2,
    KeyCtrl_C    = 3,
	KeyCtrl_D    = 4,
	KeyCtrl_E    = 5,
	KeyCtrl_F    = 6,
	KeyCtrl_H    = 8,
	KeyTab       = 9,
	KeyCtrl_K    = 11,
	KeyCtrl_L    = 12,
    KeyEnter     = 13,
	KeyCtrl_N    = 14,
	KeyCtrl_P    = 16,
	KeyCtrl_T    = 20,
	KeyCtrl_U    = 21,
	KeyCtrl_W    = 23,
	KeyEsc       = 27,
	KeySpace     = 32,
	KeyBackSpace = 127
} KeyStroke;


static inline Err switch_tty_to_raw_mode(struct termios prev_termios[static 1]) {

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


char* reditline(const char* prompt, char* line);
bool reditline_error(char* res);
int redit_history_add(const char* line);
#endif
