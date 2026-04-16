#ifndef __AHRE_CMD_PARAMS_H__
#define __AHRE_CMD_PARAMS_H__

#include "cmd-out.h"
#include "ranges.h"

typedef struct Session Session;
typedef struct TextBuf TextBuf;

typedef struct {
    const char* ln;
    Session*    s;
    TextBuf*    tb; /* in order to reuse buf cmds for source & buf, we pass it */
    RangeParse  rp;
    CmdOut*     out;
} CmdParams;

static inline CmdOut* cmd_params_cmd_out(CmdParams p[_1_]) { return p->out; }
const char* cmd_params_match(CmdParams p[_1_], const char* cmd_name, size_t unmatch);

#define get_cmd_out_(P) _Generic((P),\
    CmdParams*: cmd_params_cmd_out,\
    CmdOut*   : cmd_out_id\
)(P)

#define msg__(P,M)       cmd_out_msg_append(get_cmd_out_(P),M)
#define msg_ln__(P,M)    cmd_out_msg_append_ln(get_cmd_out_(P),M)
#define screen__(P,M)    cmd_out_screen_append(get_cmd_out_(P),M)
#define screen_ln__(P,M) cmd_out_screen_append_ln(get_cmd_out_(P),M)

#endif
