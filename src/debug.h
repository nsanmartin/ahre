#ifndef __AHRE_DEBUG_H__
#define __AHRE_DEBUG_H__

#include "src/session.h"

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

static LogLvl _log_lvl_ = LOG_LVL_TODO;
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

static inline void log_lvl__(LogLvl level, const char* format, ...) {
    if (_log_lvl_ < level) return;
    printf("%s", _log_lvl_str_[level]); 
    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
    puts("");
}


#define log_warn__(Fmt, ...) log_lvl__(LOG_LVL_WARN, Fmt, __VA_ARGS__)
#define log_info__(Fmt, ...) log_lvl__(LOG_LVL_INFO, Fmt, __VA_ARGS__)
#define log_debug__(Fmt, ...) log_lvl__(LOG_LVL_DEBUG, Fmt, __VA_ARGS__)
#define log_todo__(Fmt, ...) log_lvl__(LOG_LVL_TODO, Fmt, __VA_ARGS__)

Err dbg_print_form(Session s[static 1], const char* line);
#endif
