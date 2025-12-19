#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"
#include "utils.h"

//static constexpr size_t MAX_MSG_LEN = 512;
#define MAX_MSG_LEN 1024u
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
    if ((size_t)bytes >= MAX_MSG_LEN) {
        // message was truncated
        bytes = MAX_MSG_LEN;
        memcpy(err_msg_buf, TRUNC_ERR, sizeof(TRUNC_ERR)-1);
    }
    memcpy(MSGBUF, err_msg_buf, bytes + 1);
    return MSGBUF;
}


static Err _msg_buf_append(
    char*  errmsgbuf,
    size_t errmsgbuflen[_1_],
    size_t errmsgbufmaxlen,
    char*  bufptr[_1_],
    const  char* s,
    size_t sz
) {
    if (*bufptr + sz >= errmsgbuf + errmsgbufmaxlen) return "error: err_fmt too large.";
    memmove(*bufptr, s, sz);
    *bufptr += sz;
    *errmsgbuflen += sz;
    return Ok;
}


static Err _msg_buf_append_num_(
    char*  errmsgbuf,
    size_t errmsgbuflen[_1_],
    size_t errmsgbufmaxlen,
    char*  bufptr[_1_],
    int    n
) {
    if (*errmsgbuflen >= errmsgbufmaxlen) return "error: err_fmt too large.";
    if (n == 0) {
        **bufptr       = '0';
        *bufptr  += 1;
        *errmsgbuflen  += 1;
        return Ok;
    } else if (n == INT_MIN) {
        const size_t intminssz = sizeof "INT_MIN" - 1;
        if (*bufptr + intminssz >= errmsgbuf + errmsgbufmaxlen) return "error: err_fmt too large.";
        memmove(*bufptr, "INT_MIN", intminssz);
        *bufptr       += intminssz;
        *errmsgbuflen += intminssz;
        return Ok;
    }

    if (n < 0) {
        n = -n;
        **bufptr = '-';
        *bufptr  += 1;
        ++*errmsgbuflen;
    }
    char* num            = *bufptr;
    while (n != 0 && *errmsgbuflen < errmsgbufmaxlen) {
        **bufptr = '0' + (n % 10);
        *bufptr  += 1;
        ++*errmsgbuflen;
        n /= 10;
    }
    if (*errmsgbuflen >= errmsgbufmaxlen)
        return "error: not enough size to convert num to str.";
    str_reverse(num, *bufptr - num);

    return Ok;
}


//TODO: deprecate
Err _err_fmt_while_(Err fmt, ...) {
    if (!fmt || !*fmt) { return "error: err_fmt fmt is empty."; }

    const size_t errmsgbufmaxlen  = MAX_MSG_LEN;
    size_t errmsgbuflen           = 0;
    char errmsgbuf[MAX_MSG_LEN+1] = {0};

    const char* beg = fmt;
    const char* end;
    char* buf = errmsgbuf;
    va_list ap;

    va_start(ap, fmt);
    errmsgbuflen = 0;     

    while ((end = strchr(beg, '%'))) {
        if (end < beg) return "error: strchr error.";
        else if (beg < end) {
            try(_msg_buf_append(errmsgbuf, &errmsgbuflen, errmsgbufmaxlen, &buf, beg, end-beg));
            beg = end + 1;
        } else if (beg == end) { ++beg; }

        const char* s = NULL;
        switch(*beg) {
            case 's':
                s = va_arg(ap, const char *);
                if (s == MSGBUF)
                    return "error: err_fmt can't receive as parameter an err_fmt return value";
                if (s == NULL) s = "(null)";
                try(_msg_buf_append(errmsgbuf, &errmsgbuflen, errmsgbufmaxlen, &buf, s, strlen(s)));
                break;

            case 'd':
                try(_msg_buf_append_num_(
                    errmsgbuf,
                    &errmsgbuflen,
                    errmsgbufmaxlen,
                    &buf,
                    va_arg(ap, int)
                ));
                break;

            case '%':
                try(_msg_buf_append(errmsgbuf, &errmsgbuflen, errmsgbufmaxlen, &buf, "%", 1));
                break;

            default:
                return "error: unsupported fmt";
        }
        ++beg;
    }

    try(_msg_buf_append(errmsgbuf, &errmsgbuflen, errmsgbufmaxlen, &buf, beg, strlen(beg)));
    memcpy(MSGBUF, errmsgbuf, errmsgbuflen + 1);
    return MSGBUF;
}
