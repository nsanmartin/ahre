#ifndef AHRE_REDITLINE_H__
#define AHRE_REDITLINE_H__

#include <termios.h>

#include "str.h"

void reditline_history_cleanup(ArlOf(const_cstr) history[_1_]);
char* reditline(const char* prompt, char* line, ArlOf(const_cstr) history[_1_]);
bool reditline_error(char* res);
int redit_history_add(ArlOf(const_cstr) history[_1_], const char* line);
Err switch_tty_to_raw_mode(struct termios prev_termios[_1_]);


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

#endif
