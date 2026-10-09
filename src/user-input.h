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
#include "ranges.h"
#include "cmd-out.h"

typedef struct {
    Str    line;
    size_t ix;
} UserLine ;

Err fail_user_line_no_parse(void);
static inline const char* user_line_full(UserLine ul[_1_]) { return ul->line.items; }
static inline void user_line_exhaust(UserLine ul[_1_]) { ul->ix = ul->line.len; }
static inline bool user_line_is_exhausted(UserLine ul[_1_]) { return ul->ix >= ul->line.len; }
static inline bool user_line_is_empty(UserLine ul[_1_]) { return ul->line.len == 0; }
static inline char user_line_current(UserLine ul[_1_]) { return ul->line.items[ul->ix]; }
static inline const char* user_line_remaining(UserLine ul[_1_]) {  return ul->line.items + ul->ix; }
static inline size_t user_line_remaining_len(UserLine ul[_1_]) { return ul->line.len - ul->ix; }
static inline Str* user_line_line(UserLine ul[_1_]) { return &ul->line; }
static inline StrView user_line_remaining_view(UserLine ul[_1_]) {
    return (StrView){.items=user_line_remaining(ul),.len=user_line_remaining_len(ul)};
}

static inline char user_line_char(UserLine ul[_1_]) { return ul->line.items[ul->ix]; }
static inline Err user_line_init_take_ownership(UserLine ul[_1_], const char* line) {
    *ul = (UserLine){.line=(Str){.items=(char*)line,.len=line?strlen(line):0}, .ix=0, };
    return Ok;
}

static inline void user_line_cleanup(UserLine ul[_1_]) {
    str_clean(user_line_line(ul));
    *ul = (UserLine){0};
}

void user_line_unskip(UserLine ul[_1_], size_t n);
void user_line_skip(UserLine ul[_1_], size_t n);
void user_line_skip_space(UserLine ul[_1_]);
bool user_line_match_char(UserLine ul[_1_], char c);
Err user_line_match_last_char(UserLine ul[_1_], char c, CmdOut cout[_1_]);
bool user_line_pop_char(UserLine ln[_1_], char out[_1_]);
bool user_line_pop_nonspace(UserLine ln[_1_], StrView out[_1_]);
bool user_line_pop_alpha(UserLine ln[_1_], StrView out[_1_]);
bool user_line_pop_digits(UserLine ln[_1_], StrView out[_1_]);
bool user_line_pop_alnum(UserLine ln[_1_], StrView out[_1_]);
Err user_line_pop_last_char(UserLine ln[_1_], char out[_1_], CmdOut cout[_1_]);
Err user_line_pop_last_nonspace(UserLine ln[_1_], StrView out[_1_], CmdOut cout[_1_]);
Err user_line_pop_last_alpha(UserLine ln[_1_], StrView out[_1_], CmdOut cout[_1_]);
Err user_line_pop_last_digits(UserLine ln[_1_], StrView out[_1_], CmdOut cout[_1_]);
Err user_line_pop_last_alnum(UserLine ln[_1_], StrView out[_1_], CmdOut cout[_1_]);
bool user_line_pop_pattern(UserLine ul[_1_], StrView pattern[_1_]);
bool user_line_pop_rest(UserLine ul[_1_], StrView rest[_1_]);
StrView user_line_word_view(UserLine ul[_1_]);
Err user_line_chop_and_skip_space(UserLine ln[_1_]);

Err user_line_parse_range(UserLine ln[_1_], int base, RangeParse out[_1_]);
static inline void user_line_skip_all(UserLine ul[_1_]) { ul->ix = ul->line.len; }
Err user_line_parse_size_t_or_throw(UserLine ln[_1_], size_t* num, int base);
static inline bool user_line_cmd_end(UserLine ul[_1_]) {
    return ul->ix >= ul->line.len
        || user_line_char(ul) == ';'
        || user_line_char(ul) == '>'
        || user_line_char(ul) == '\0'; 
}
bool user_line_cmd_end_skipping_space(UserLine ul[_1_]);
Err user_line_cut_cmd(UserLine ul[_1_], CmdOut cout[_1_]);


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
        char* realloc_res = std_realloc(*out, len);
        if (!realloc_res) {
            std_free(*out);
            *out = NULL;
            return "error: realloc failure in fgets readline";
        }
        *out = realloc_res;
        char* line = fgets(*out + readlen, cast__(int)(len - readlen), stdin);
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

Err ui_vi_flush_msg_read_input(Session* s, StrView msg);
Err wait_for_char(char c);
#endif

