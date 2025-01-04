#ifndef __USER_INPUT_AH_H__
#define __USER_INPUT_AH_H__

/* readline version, deprecated */
//#include <readline/history.h>
//#include <readline/readline.h>
//#define init_user_input_history
// static inline char* read_user_input(const char* prompt) { return readline(prompt); }
// static inline void add_to_user_input_history(const char* line) { add_history(line); }


#include "isocline.h"

static inline void init_user_input_history(void) {
    ic_set_prompt_marker("", "");
    ic_enable_brace_insertion(false);
    ic_set_history(NULL, -1);
}
static inline char* read_user_input(const char* prompt) { return ic_readline(prompt); }
#define add_to_user_input_history(X) 

#endif

