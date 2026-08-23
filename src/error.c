#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"
#include "utils.h"

/* size_t MAX_MSG_LEN = 512; */
#define TRUNC_ERR "TRERR:"

/* _Thread_local */ size_t ERR_MSG_LEN = 0;
/* _Thread_local */ char MSGBUF[MAX_MSG_LEN+1] = {0};



Err err_fmt_buf(char* buf, size_t len, Err fmt, ...) {
    if (len > MAX_MSG_LEN) return "error: error message buffer greater than MAX_MSG_LEN";
    va_list ap;
    va_start(ap, fmt);
    const char* beg = fmt;
    const char* end;
    for (;(end = strchr(beg, '%')); ++beg) {
        if (beg > end) return "error: strchr error.";
        else if (beg <= end) { beg = end + 1; }

        if(*beg == 's') {
            if (va_arg(ap, const char *) == buf)
                return "error: err_fmt can't receive as parameter an err_fmt return value";
        } else  va_arg(ap, int);
    }
    va_end(ap);

    char err_msg_buf[MAX_MSG_LEN] = {0};
    va_start(ap, fmt);
    int bytes = vsnprintf(err_msg_buf, len, fmt, ap);
    va_end(ap);
    if (bytes < 0) return "error: while processing another error mesage, a failure was produced";
    if (cast__(size_t)bytes >= len) {
        // message was truncated
        bytes = len - 1;
        memcpy(err_msg_buf, TRUNC_ERR, sizeof(TRUNC_ERR)-1);
        err_msg_buf[bytes] = '\0';
    }
    memcpy(buf, err_msg_buf, cast__(size_t)bytes + 1);
    return buf;
}



Err
err_from_cstr(const char* msg) { return msg; }

Err
err_from_errno(int en) { return strerror(en); }

#ifdef AHRE_SIMULATE_ERR
static size_t ntry = 0;
bool simulate_error(void) { return ++ntry % AHRE_SIMULATE_ERR == 0; }
# endif
