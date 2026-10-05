#include <stdarg.h>
#include "cmd-out.h"
#include "error.h"
#include "generic.h"
#include "sys.h"


Err msg_fmt(CmdOut cout[_1_], const char* fmt, ...) {
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
cmd_out_sink_to_file(CmdOutSink sink[_1_], FilePtr fp[_1_]) {
    switch (sink->tag) {
        case CMD_OUT_STD:
        case CMD_OUT_MSG:
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
        case CMD_OUT_STD:
        case CMD_OUT_MSG:
            break;
        case CMD_OUT_FILE:
            try(file_close(fp->ptr));
            break;
        default:
            err = "error: invalid CmdOutSinkTag";
    }
    *sink = (CmdOutSink){.tag=CMD_OUT_STD};
    return err;
}

/* Err */
/* cmd_out_std_flush(CmdOut co[_1_]) { */
/*     Str* screen = cmd_out_screen(co); */
/*     if (!len__(screen)) return Ok; */

/*     FilePtr fp; */
/*     try(cmd_out_sink_to_file(&co->std_sink, &fp)); */
/*     try( mem_fwrite(items__(screen), len__(screen), fp.ptr)); */
/*     if (fflush(fp.ptr)) return err_fmt("error: fflush failure: %s", strerror(errno)); */
/*     str_reset(screen); */
/*     cmd_out_sink_close_file(&co->std_sink, &fp); */
/*     return Ok; */
/* } */


