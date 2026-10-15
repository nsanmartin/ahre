#include <stdarg.h>
#include "cmd-out.h"
#include "error.h"
#include "generic.h"
#include "sys.h"


Err
msg_fmt(CmdOut cout[_1_], const char* fmt, ...) {
#define TRUNC_MSG "TRMsg:"

    char msg_buf[MAX_MSG_LEN+1] = {0};
    va_list ap;
    va_start(ap, fmt);
    int bytes = vsnprintf(msg_buf, MAX_MSG_LEN, fmt, ap);
    va_end(ap);
    if (bytes < 0) return "error: failure while processing a mesage";
    if (cast__(size_t)bytes >= MAX_MSG_LEN) {
        // message was truncated
        bytes = MAX_MSG_LEN;
        memcpy(msg_buf, TRUNC_MSG, sizeof(TRUNC_MSG)-1);
    }
    return msg__(cout, sv(msg_buf));
}


Err
cmd_out_sink_cleanup(CmdOutSink sink[_1_], CmdOutSinkTag tag) {
    switch (sink->tag) {
        case CMD_OUT_SCREEN:
        case CMD_OUT_WARN:
            break;
        case CMD_OUT_FILE:
            str_clean(&sink->filename);
            break;
        case CMD_OUT_PROC:
            str_clean(&sink->proc_cmd);
            break;
        default:
            fail_e("invalid sink tag!");
    }

    *sink = (CmdOutSink){.tag=tag};
    return Ok;
}


Err
cmd_out_sink_set(CmdOutSink sink[_1_], CmdOutSinkTag tag, StrView* s) {
    try(cmd_out_sink_cleanup(sink, tag));
    switch (tag) {
        case CMD_OUT_SCREEN:
        case CMD_OUT_WARN:
            return Ok;
        case CMD_OUT_FILE:
            if (!s) fail_e("null ptr failure");
            try(str_append(&sink->filename, s));
            return Ok;
        case CMD_OUT_PROC:
            if (!s) fail_e("null ptr failure");
            try(str_append(&sink->proc_cmd, s));
            return Ok;
        default:
            fail_e("invalid sink tag!");
    }
}


Err
cmd_out_sink_to_file(CmdOutSink sink[_1_], FilePtr fp[_1_]) {
    switch (sink->tag) {
        case CMD_OUT_SCREEN:
        case CMD_OUT_WARN:
            fp->ptr = stdout;
            return Ok;
        case CMD_OUT_FILE:
            return file_open(sink->filename.items,"w", &fp->ptr);
        default:
            fail_e("invalid CmdOutSinkTag");
    }
}

Err
cmd_out_sink_close_file(CmdOutSink sink[_1_], FilePtr fp[_1_]) {
    Err err = Ok;
    switch (sink->tag) {
        case CMD_OUT_SCREEN:
        case CMD_OUT_WARN:
            break;
        case CMD_OUT_FILE:
            try(file_close(fp->ptr));
            break;
        default:
            err = "error: invalid CmdOutSinkTag";
    }
    *sink = (CmdOutSink){.tag=CMD_OUT_SCREEN};
    return err;
}


bool
cmd_out_will_touch_screen(CmdOut o[_1_]) {
    return
       (o->std_sink.tag == CMD_OUT_SCREEN && cmd_out_has_std(o))
    || (o->msg_sink.tag == CMD_OUT_SCREEN && cmd_out_has_msg(o))
        ;
}
