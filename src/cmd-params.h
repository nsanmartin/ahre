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

#endif
