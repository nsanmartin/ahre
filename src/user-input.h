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

#include "error.h"
#include "isocline.h"

typedef struct Session Session;

typedef Err (*InitUserInputCallback)(void);
typedef Err (*ReadUserInputCallback)(Session* s, const char* prompt, char* line[_1_]);

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

static inline Err ui_isocline_readline(Session* s, const char* prompt, char* out[_1_]) {
    (void)s;
    *out = ic_readline(prompt);
    return Ok;
}

/* fgets */
static inline Err ui_fgets_readline(Session* s, const char* prompt, char* out[_1_]) {
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

Err ui_vi_mode_read_input(Session* s, const char* prompt, char* out[_1_]);

static inline UserInput uinput_vi_mode(void) {
    return (UserInput){.init=err_skip, .read=ui_vi_mode_read_input};
}

#endif

