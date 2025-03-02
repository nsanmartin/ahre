#ifndef __USER_INPUT_AH_H__
#define __USER_INPUT_AH_H__

typedef enum {
    uiin_getline,
    uiin_isocline
} UiIn;

#include "isocline.h"

#include "src/error.h"

static inline void init_user_input_history(void) {
    ic_set_prompt_marker("", "");
    ic_enable_brace_insertion(false);
    ic_set_history(NULL, -1);
}

static inline Err read_user_input(UiIn uiin, const char* prompt, char* out[static 1]) {
    Err err = Ok;
    switch (uiin) {
        case uiin_isocline: *out = ic_readline(prompt); break;
        case uiin_getline: *out = ic_readline(prompt); break;
        default: err = "error: invalid user interface input type";
    }
    return err;
}
#define add_to_user_input_history(X) 

#endif

