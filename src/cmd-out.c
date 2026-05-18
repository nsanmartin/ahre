#include <stdarg.h>
#include "cmd-out.h"
#include "error.h"
#include "generic.h"


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

