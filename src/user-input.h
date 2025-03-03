#ifndef __USER_INPUT_AH_H__
#define __USER_INPUT_AH_H__

#include <errno.h>
#include <stdio.h>

typedef enum {
    uiin_fgets,
    uiin_isocline
} UiIn;

#include "isocline.h"

#include "src/error.h"

static inline void init_user_input_history(void) {
    ic_set_prompt_marker("", "");
    ic_enable_brace_insertion(false);
    ic_set_history(NULL, -1);
}

static inline Err ui_fgets_readline(const char* prompt, char* out[static 1]) {
    if (prompt) fwrite(prompt, 1, strlen(prompt), stdout);
    const size_t DEFAULT_FGETS_SIZE = 256;
    size_t len = DEFAULT_FGETS_SIZE;
    size_t readlen = 0;
    *out = NULL;

    while (1) {
        *out = realloc(*out, len);
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

static inline Err read_user_input(UiIn uiin, const char* prompt, char* out[static 1]) {
    Err err = Ok;
    switch (uiin) {
        case uiin_isocline: *out = ic_readline(prompt); break;
        case uiin_fgets: err = ui_fgets_readline(prompt, out); break;
        default: err = "error: invalid user interface input type";
    }
    return err;
}
#define add_to_user_input_history(X) 

#endif

