#ifndef __AHRE_CMD_OUT_H__
#define __AHRE_CMD_OUT_H__

#include "str.h"
#include "nongeneric.h"
#include "utils.h"
#include "sys.h"


typedef struct { Str s; } Msg;
static inline Str* msg_str(Msg m[_1_]) { return &m->s; }

#define msg_append(Mptr, S)                str_append(msg_str(Mptr), S)
#define msg_append_ln(Mptr, S)             str_append_ln(msg_str(Mptr), S)

#define msg_clean(Mptr) str_clean(msg_str(Mptr))
#define msg_reset(Mptr) str_reset(msg_str(Mptr))
#define msg_items(Mptr) items__(msg_str(Mptr))
#define msg_len(Mptr)   len__(msg_str(Mptr))

#define msg_append_ui_as_base36(M,U) str_append_ui_as_base36(msg_str(M), U)
#define msg_append_ui_as_base10(M,U) str_append_ui_as_base10(msg_str(M), U)

// could be the session std/msg outuput or a filename
// nullptr means the standard, otherwise is a filename, 
// we could add other cases in the future (other command
// input, sockets, etc).

/* The CmdOut together with the user interface determing the output flow.
 * vi and line mode manage differently std and msg so if a user redirects
 * the msg to std will mean different things
 * */
typedef enum {
    CMD_OUT_STD  = 1,
    CMD_OUT_MSG  = 2,
    CMD_OUT_FILE = 3,
    CMD_OUT_PROC = 4
} CmdOutSinkTag;


typedef struct {
    CmdOutSinkTag tag;
    union {
        Str filename;
        Str proc_cmd;
    };
} CmdOutSink;

Err cmd_out_sink_to_file(CmdOutSink sink[_1_], FilePtr fp[_1_]);
Err cmd_out_sink_close_file(CmdOutSink sink[_1_], FilePtr fp[_1_]);

typedef struct CmdOut {
    Str        std;
    CmdOutSink std_sink;
    Msg        msg;
    CmdOutSink msg_sink;
} CmdOut;


#define cmd_out_default() \
    (CmdOut){.std_sink=(CmdOutSink){.tag=CMD_OUT_STD}, .msg_sink=(CmdOutSink){.tag=CMD_OUT_MSG}}
static inline CmdOutSinkTag cmd_out_std_sink_tag(CmdOut co[_1_]) { return co->std_sink.tag; }
static inline CmdOutSinkTag cmd_out_msg_sink_tag(CmdOut co[_1_]) { return co->msg_sink.tag; }

static inline CmdOut* cmd_out_id(CmdOut o[_1_]) { return o; }
static inline Str* cmd_out_screen(CmdOut o[_1_]) { return &o->std; }
static inline Msg* cmd_out_msg(CmdOut o[_1_]) { return &o->msg; }
static inline bool cmd_out_is_empty(CmdOut o[_1_]) {
    return !len__(cmd_out_screen(o)) && !len__(msg_str(cmd_out_msg(o)));
}

static inline void cmd_out_clean(CmdOut o[_1_]) {
    str_clean(cmd_out_screen(o));
    str_clean(msg_str(cmd_out_msg(o)));
}

/* Msg fns */
#define cmd_out_msg_append(M, S)             msg_append(cmd_out_msg(M), S)
#define cmd_out_msg_append_ln(M, S)          msg_append_ln (cmd_out_msg(M), S)
#define cmd_out_msg_append_ui_as_base10(M,U) msg_append_ui_as_base10(cmd_out_msg(M), U)
#define cmd_out_msg_append_ui_as_base36(M,U) msg_append_ui_as_base36(cmd_out_msg(M), U)


/* std fns */
#define cmd_out_screen_append(M, S)             str_append(cmd_out_screen(M), S)
#define cmd_out_screen_append_ln(M, S)          str_append_ln(cmd_out_screen(M), S)
#define cmd_out_screen_append_ui_as_base10(M,U) str_append_ui_as_base10(cmd_out_screen(M), U)
#define cmd_out_screen_append_ui_as_base36(M,U) str_append_ui_as_base36(cmd_out_screen(M), U)

Err msg_fmt(CmdOut cout[_1_], const char* fmt, ...);
Err cmd_out_std_flush(CmdOut co[_1_]);
Err cmd_out_msg_flush(CmdOut co[_1_]);
#endif
