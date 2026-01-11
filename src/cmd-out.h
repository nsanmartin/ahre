#ifndef __AHRE_CMD_OUT_H__
#define __AHRE_CMD_OUT_H__

#include "str.h"

typedef struct { Str s; } Msg;
static inline Str* msg_str(Msg m[_1_]) { return &m->s; }

#define msg_append(Mptr, Items, Nitems)    str_append(msg_str(Mptr), Items, Nitems)
#define msg_append_ln(Mptr, Items, Nitems) str_append_ln(msg_str(Mptr), Items, Nitems)
#define msg_append_lit__(Mptr, Lit)        str_append_lit__(msg_str(Mptr), Lit)

#define msg_append_str(Mptr, S)            str_append_str(msg_str(Mptr), S)
#define msg_append_str_ln(Mptr, S)         str_append_str_ln(msg_str(Mptr), S)

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

typedef struct CmdOut {
    Str std;
    Msg msg;
} CmdOut;

static inline Str* cmd_out_std(CmdOut o[_1_]) { return &o->std; }
static inline Msg* cmd_out_msg(CmdOut o[_1_]) { return &o->msg; }
static inline bool cmd_out_is_empty(CmdOut o[_1_]) {
    return !len__(cmd_out_std(o)) && !len__(msg_str(cmd_out_msg(o)));
}

static inline void cmd_out_clean(CmdOut o[_1_]) {
    str_clean(cmd_out_std(o));
    str_clean(msg_str(cmd_out_msg(o)));
}

/* Msg fns */
#define cmd_out_msg_append_ln(M, Items, Nitems) msg_append_ln(cmd_out_msg(M), Items, Nitems)

#define cmd_out_msg_append_lit__(M, S)  msg_append_lit__(cmd_out_msg(M), S)
#define cmd_out_msg_append_str(M,S)     msg_append_str(cmd_out_msg(M), S)
#define cmd_out_msg_append_str_ln(M, S) msg_append_str_ln(cmd_out_msg(M), S)
#define cmd_out_msg_append(M, Items, Nitems) msg_append (cmd_out_msg(M), Items, Nitems)

#define cmd_out_msg_append_ui_as_base10(M,U) msg_append_ui_as_base10(cmd_out_msg(M), U)
#define cmd_out_msg_append_ui_as_base36(M,U) msg_append_ui_as_base36(cmd_out_msg(M), U)


/* std fns */
//TODO: change std/screen
#define cmd_out_std_append(M, Items, Nitems)    str_append(cmd_out_std(M), Items, Nitems)
#define cmd_out_std_append_ln(M, Items, Nitems) str_append_ln(cmd_out_std(M), Items, Nitems)

#define cmd_out_std_append_lit__(M, S)  str_append_lit__(cmd_out_std(M), S)
#define cmd_out_std_append_str(M,S)     str_append_str(cmd_out_std(M), S)
#define cmd_out_std_append_str_ln(M, S) str_append_str_ln(cmd_out_std(M), S)

#define cmd_out_std_append_ui_as_base10(M,U) str_append_ui_as_base10(cmd_out_std(M), U)
#define cmd_out_std_append_ui_as_base36(M,U) str_append_ui_as_base36(cmd_out_std(M), U)

#endif
