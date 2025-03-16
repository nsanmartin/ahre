#ifndef __AHRE_DEBUG_H__
#define __AHRE_DEBUG_H__

#include "src/session.h"

#define MAX_LOG_MSG_LEN__ 4056

static inline bool is_debug_build(void) {
#ifndef DEBUG 
    return true;
#else
    return false;
#endif
}

typedef enum {
    LOG_LVL_FATAL,
    LOG_LVL_ERROR,
    LOG_LVL_WARN,
    LOG_LVL_INFO,
    LOG_LVL_DEBUG,
    LOG_LVL_TRACE,
    LOG_LVL_TODO
} LogLvl;

#ifdef DEBUG
static LogLvl _log_lvl_ = LOG_LVL_TODO;
#else
static LogLvl _log_lvl_ = LOG_LVL_WARN;
#endif

#define try_debug_build_only() do{ if (is_debug_build()) return "not a debug build"; }while(0);

static inline void dbg_log(const char* s) { printf("DEBUG %s\n", s); }
static inline void dbg_print_url(Url u[static 1]) {
    char* buf;
    Err e = url_cstr(u, &buf);
    if (e) printf("DEBUG error: %s\n", e);
    else {
        printf("DEBUG url: %s\n", buf);
        curl_free(buf);
    }
}


static const char* _log_lvl_str_[] = {
    [LOG_LVL_FATAL] = "[fatal] ",
    [LOG_LVL_ERROR] = "[error] ",
    [LOG_LVL_WARN]  = "[warn] ",
    [LOG_LVL_INFO]  = "[info] ",
    [LOG_LVL_DEBUG] = "[debug] ",
    [LOG_LVL_TRACE] = "[trace] ",
    [LOG_LVL_TODO]  = "[todo] " 
};

static inline Err
_log_format__(SessionWriteFn wc, const char* format, va_list args) {
    if (!wc.write) return "error: expectinf write fn";

    char log_buf[MAX_LOG_MSG_LEN__] = {0};

    int written = vsnprintf (log_buf, MAX_LOG_MSG_LEN__, format, args);

    if (written >= MAX_LOG_MSG_LEN__) {
        char* msg = ":( log message too long\n";
        try (wc.write(msg, strlen(msg), wc.ctx));
    } else {
        try (wc.write(log_buf, written, wc.ctx));
    }
    return Ok;
}



static inline Err log_lvl__(SessionWriteFn wc, LogLvl level, const char* format, ...) {
    if (!wc.write || _log_lvl_ < level) return Ok;
    try (wc.write(_log_lvl_str_[level], strlen(_log_lvl_str_[level]), wc.ctx));

    va_list args;
    va_start (args, format);
    try( _log_format__(wc, format, args));
    va_end (args);
    return Ok;
}

static inline Err log_msg__(SessionWriteFn wc, const char* format, ...) {
    if (!wc.write) return Ok;
    va_list args;
    va_start (args, format);
    try( _log_format__(wc, format, args));
    va_end (args);
    return Ok;
}

#define log_warn__(Wcb, Fmt, ...) log_lvl__(Wcb, LOG_LVL_WARN, Fmt, __VA_ARGS__)
#define log_info__(Wcb, Fmt, ...) log_lvl__(Wcb, LOG_LVL_INFO, Fmt, __VA_ARGS__)
#define log_debug__(Wcb, Fmt, ...) log_lvl__(Wcb, LOG_LVL_DEBUG, Fmt, __VA_ARGS__)
#define log_todo__(Wcb, Fmt, ...) log_lvl__(Wcb, LOG_LVL_TODO, Fmt, __VA_ARGS__)

Err dbg_print_form(Session s[static 1], const char* line);
#endif
