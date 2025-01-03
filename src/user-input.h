#ifndef __USER_INPUT_AH_H__
#define __USER_INPUT_AH_H__
#include <readline/history.h>
#include <readline/readline.h>

static inline char* read_user_input(const char* prompt) { return readline(prompt); }
static inline void add_to_user_input_history(const char* line) { add_history(line); }
#endif

