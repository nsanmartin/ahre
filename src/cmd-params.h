#ifndef __AHRE_CMD_PARAMS_H__
#define __AHRE_CMD_PARAMS_H__

#include "cmd-out.h"
#include "ranges.h"
#include "user-input.h"

typedef struct Session Session;
typedef struct TextBuf TextBuf;

typedef struct {
    UserLine   ln;
    Session*   s;
    TextBuf*   tb; /* in order to reuse buf cmds for source & buf, we pass it */
    RangeParse rp;
    CmdOut*    out;
} CmdParams;

static inline UserLine* cmd_params_user_line(CmdParams p[_1_]) { return &p->ln; }
static inline CmdOut* cmd_params_cmd_out(CmdParams p[_1_]) { return p->out; }
// const char* cmd_params_match_substring(CmdParams p[_1_], const char* cmd_name, size_t unmatch);
bool cmd_match_substring(StrView input, const char* cmd_name, size_t unmatch, CmdParams p[_1_]);

static inline bool cmd_params_match_char(CmdParams p[_1_], char c) {
    return user_line_match_char(cmd_params_user_line(p), c);
}


static inline void cmd_params_skip_space(CmdParams p[_1_]) { user_line_skip_space(cmd_params_user_line(p)); }
static inline void cmd_params_skip(CmdParams p[_1_], size_t n) {
    user_line_skip(cmd_params_user_line(p), n);
}
static inline bool cmd_params_cmd_end_skipping_space(CmdParams p[_1_]) {
    return user_line_cmd_end_skipping_space(cmd_params_user_line(p));
}

static inline bool cmd_params_cmd_end(CmdParams p[_1_]) { return user_line_cmd_end(cmd_params_user_line(p)); }
static inline bool cmd_params_pop_char(CmdParams p[_1_], char out[_1_]) {
    return user_line_pop_char(cmd_params_user_line(p), out);
}
static inline bool cmd_params_pop_last_char(CmdParams p[_1_], char out[_1_]) {
    return user_line_pop_last_char(cmd_params_user_line(p), out);
}

#define cmd_params_pop_last_path cmd_params_pop_last_word

static inline bool cmd_params_pop_word(CmdParams p[_1_], StrView out[_1_]) {
    return user_line_pop_word(cmd_params_user_line(p), out);
}

static inline bool cmd_params_pop_last_word(CmdParams p[_1_], StrView out[_1_]) {
    return user_line_pop_last_word(cmd_params_user_line(p), out);
}

static inline bool cmd_params_pop_pattern(CmdParams p[_1_], StrView pattern[_1_]) {
    return user_line_pop_pattern(cmd_params_user_line(p), pattern);
}

static inline bool cmd_params_pop_rest(CmdParams p[_1_], StrView rest[_1_]) {
    return user_line_pop_rest(cmd_params_user_line(p), rest);
}

static inline Err cmd_params_parse_size_t_or_throw(CmdParams p[_1_], size_t ncols[_1_], int base) {
    return user_line_parse_size_t_or_throw(cmd_params_user_line(p), ncols, base);
}

static inline bool cmd_params_cut_cmd(CmdParams p[_1_]) {
    return user_line_cut_cmd(cmd_params_user_line(p));
}

static inline StrView cmd_params_word_view(CmdParams p[_1_]) {
    return user_line_word_view(cmd_params_user_line(p));
}

static inline bool cmd_params_match_last_char(CmdParams p[_1_], char c) {
    return user_line_match_last_char(cmd_params_user_line(p), c);
}

bool cmd_match(StrView input, const char* cmd_name, size_t unmatch, CmdParams p[_1_]);

static inline Err cmd_params_parse_range(CmdParams p[_1_], int base) {
    return user_line_parse_range(cmd_params_user_line(p), base, &p->rp);
}
#endif
