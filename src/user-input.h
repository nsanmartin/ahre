#ifndef __USER_INPUT_AH_H__
#define __USER_INPUT_AH_H__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "isocline.h"

#include "src/error.h"

static inline Err _switch_tty_to_raw_mode_(struct termios prev_termios[static 1]) {

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
	KeyBackspace = 127
} KeyStroke;

typedef struct Session Session;

typedef Err (*InitUserInputCallback)(void);
typedef Err (*ReadUserInputCallback)(Session* s, const char* prompt, char* line[static 1]);

typedef struct {
    InitUserInputCallback init;
    ReadUserInputCallback read;
} UserInput;

/* isocline */
static inline Err init_user_input_history(void) {
    ic_set_prompt_marker("", "");
    ic_enable_brace_insertion(false);
    ic_set_history(NULL, -1);
    return Ok; //TODO: check errors
}

static inline Err ui_isocline_readline(Session* s, const char* prompt, char* out[static 1]) {
    (void)s;
    *out = ic_readline(prompt);
    return Ok;
}

/* fgets */
static inline Err ui_fgets_readline(Session* s, const char* prompt, char* out[static 1]) {
    (void)s;
    if (prompt) fwrite(prompt, 1, strlen(prompt), stdout);
    const size_t DEFAULT_FGETS_SIZE = 256;
    size_t len = DEFAULT_FGETS_SIZE;
    size_t readlen = 0;
    *out = NULL;

    while (1) {
        *out = std_realloc(*out, len);
        if (!*out) return "error: realloc failure";
        char* line = fgets(*out + readlen, len - readlen, stdin);
        if (!line) {
            if (feof(stdin)) { clearerr(stdin); *out[0] = '\0'; return Ok; }
            return err_fmt("error: fgets failure: %s", strerror(errno));
        }
        if (strchr(line, '\n')) return Ok;
        readlen = len - 1;
        len += DEFAULT_FGETS_SIZE;
    }
}

#define add_to_user_input_history(X) 

static inline UserInput uinput_isocline(void) {
    return (UserInput){.init=init_user_input_history, .read=ui_isocline_readline};
}

static inline UserInput uinput_fgets(void) {
    return (UserInput){.init=err_skip, .read=ui_fgets_readline};
}

Err ui_vi_mode_read_input(Session* s, const char* prompt, char* out[static 1]);

static inline UserInput uinput_vi_mode(void) {
    return (UserInput){.init=err_skip, .read=ui_vi_mode_read_input};
}

#endif

