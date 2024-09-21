#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <ah/error.h>

static constexpr size_t MAX_MSG_LEN = 512;
thread_local size_t ERR_MSG_LEN = 0;
thread_local char MSGBUF[MAX_MSG_LEN+1] = {0};

static size_t count_specifiers(const char* it) {
    size_t count = 0;
    const char* s = "%s";
    for(;;) {
        it = strstr(it, s);
        if (!it) { break; }
        if(*++it == 's') ++count;
    }

    return count;
}


Err err_fmt(Err fmt, ...) {
    if (count_specifiers(fmt) == 0) return fmt;
    va_list ap;
    va_start(ap, fmt);
    int bytes = vsnprintf(MSGBUF, MAX_MSG_LEN, fmt, ap);
    va_end(ap);
    if (bytes < 0) {
        return "while processing another error mesage, a failure was produced";
    }
    ERR_MSG_LEN = bytes;     
    MSGBUF[MAX_MSG_LEN] = '\0';
    return MSGBUF;
}

