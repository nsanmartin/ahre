#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"
#include "utils.h"

//static constexpr size_t MAX_MSG_LEN = 512;
#define MAX_MSG_LEN 4090u
#define TRUNC_ERR "TRERR:"

/* _Thread_local */ size_t ERR_MSG_LEN = 0;
/* _Thread_local */ char MSGBUF[MAX_MSG_LEN+1] = {0};



Err _err_fmt_vsnprinf_(Err fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char* beg = fmt;
    const char* end;
    for (;(end = strchr(beg, '%')); ++beg) {
        if (beg > end) return "error: strchr error.";
        else if (beg <= end) { beg = end + 1; }

        if(*beg == 's') {
            if (va_arg(ap, const char *) == MSGBUF)
                return "error: err_fmt can't receive as parameter an err_fmt return value";
        } else  va_arg(ap, int);
    }
    va_end(ap);

    char err_msg_buf[MAX_MSG_LEN+1] = {0};
    va_start(ap, fmt);
    int bytes = vsnprintf(err_msg_buf, MAX_MSG_LEN, fmt, ap);
    va_end(ap);
    if (bytes < 0) return "error: while processing another error mesage, a failure was produced";
    if (cast__(size_t)bytes >= MAX_MSG_LEN) {
        // message was truncated
        bytes = MAX_MSG_LEN;
        memcpy(err_msg_buf, TRUNC_ERR, sizeof(TRUNC_ERR)-1);
    }
    memcpy(MSGBUF, err_msg_buf, cast__(size_t)bytes + 1);
    return MSGBUF;
}

