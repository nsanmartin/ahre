#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"
#include "utils.h"

//static constexpr size_t MAX_MSG_LEN = 512;
#define MAX_MSG_LEN 1024u
/* _Thread_local */ size_t ERR_MSG_LEN = 0;
/* _Thread_local */ char MSGBUF[MAX_MSG_LEN+1] = {0};


/*
 * err_fmt only accepts %s and %d format specifiers.
 */
static Err _err_fmt_count_params(const char* s, size_t nparams[static 1]) {
   nparams = 0;
   while ((s = strchr(s, '%'))) {
       ++s;
       if (*s == 's' || *s == 'd') {
           ++*nparams;
           continue;
       } else if (*s == '%') { 
           continue;
       }
       return "error: invalid err_fmt format string.";
   }
   return Ok;
}


Err _is_err_fmt_valid(Err fmt, ...) {
    size_t nparams;
    try(_err_fmt_count_params(fmt, &nparams));
    va_list ap;
    va_start(ap, fmt);
    while (nparams--) {
        if (MSGBUF == va_arg(ap, char *)) {
            return "error: err_fmt cannot receive as parameter a return value from err_fmt";
        }
    }
    va_end(ap);
    return Ok;
}

/*
 * err_fmt cannot use an err_fmt return value as parameter.
 */
Err err_fmt_(Err fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int bytes = vsnprintf(MSGBUF, MAX_MSG_LEN, fmt, ap);
    va_end(ap);
    if (bytes < 0) {
        return "error: while processing another error mesage, a failure was produced";
    }
    ERR_MSG_LEN = bytes;     
    MSGBUF[MAX_MSG_LEN] = '\0';
    return MSGBUF;
}

static Err _msg_buf_append(char* buf[static 1], const char* s, size_t sz) {
    if (*buf + sz >= MSGBUF + MAX_MSG_LEN) return "error: err_fmt too large.";
    memmove(*buf, s, sz);
    *buf += sz;
    ERR_MSG_LEN += sz;
    return Ok;
}

static Err _num_to_str(char* buf, size_t sz, int n, size_t len[static 1]) {
    if (!sz) return "error: not enough size to convert num to str.";
    if (n == 0) {
        *buf = '0';
        *len = 1;
        return Ok;
    } else if (n == INT_MIN) {
        const size_t intminssz = sizeof "INT_MIN" - 1;
        if (buf + intminssz >= MSGBUF + MAX_MSG_LEN) return "error: err_fmt too large.";
        memmove(buf, "INT_MIN", intminssz);
        //buf += intminssz;
        *len += intminssz;
        return Ok;
    }
    *len = 0;
    bool negative = n < 0;
    char* num = buf;

    if (negative) {
        n = -n;
        *buf++ = '-';
        ++*len;
    }
    while (n != 0 && sz > 0) {
        --sz;
        ++*len;
        *buf++ = '0' + (n % 10);
        n /= 10;
    }
    if (sz == 0 && n != 0) return "error: not enough size to convert num to str.";
    str_reverse(num + negative, *len - negative);
    return Ok;
}

Err err_prepend_char(Err err, char c) {
    if (MSGBUF != err) {
        MSGBUF[0] = c;
        ERR_MSG_LEN = 1;
        char* buf = MSGBUF + 1;
        try( _msg_buf_append(&buf, err, strlen(err)));
        MSGBUF[ERR_MSG_LEN] = '\0';
        return MSGBUF;
    }
    if (ERR_MSG_LEN == MAX_MSG_LEN)
        return "error: while returning error, not enought size to prepend to err buf";
    memmove(MSGBUF, MSGBUF + 1, ERR_MSG_LEN);
    MSGBUF[0] = c;
    ++ERR_MSG_LEN;
    MSGBUF[ERR_MSG_LEN] = '\0';
    return MSGBUF;
}

Err err_fmt(Err fmt, ...) {
    if (!fmt || !*fmt) { return "error: err_fmt fmt is empty."; }
    const char* beg = fmt;
    const char* end;
    char* buf = MSGBUF;
    va_list ap;

    va_start(ap, fmt);
    ERR_MSG_LEN = 0;     

    while ((end = strchr(beg, '%'))) {
        if (end < beg) return "error: strchr error.";
        else if (beg < end) {
            try(_msg_buf_append(&buf, beg, end-beg));
            beg = end + 1;
        } else if (beg == end) { ++beg; }

        switch(*beg) {
            case 's': {
                const char* s = va_arg(ap, const char *);
                if (s == MSGBUF)
                    return "error: err_fmt can't receive as parameter an err_fmt return value";
                if (s == NULL) s = "(null)";
                try(_msg_buf_append(&buf, s, strlen(s)));
                break;
                      }
            case 'd':{
                size_t len = 0;
                try(_num_to_str(buf, MSGBUF + MAX_MSG_LEN - buf, va_arg(ap, int), &len));
                buf += len;
                ERR_MSG_LEN += len;
                break;
                     }
            case '%':{
                try(_msg_buf_append(&buf, "%", 1));
                break;
                     }
            default:
                return "error: unsupported fmt";
        }
        ++beg;
    }

    try(_msg_buf_append(&buf, beg, strlen(beg)));

    MSGBUF[ERR_MSG_LEN] = '\0';
    return MSGBUF;
}

