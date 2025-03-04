#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

#include "src/utils.h"

typedef enum {
    uiout_stdout
} UiOut;


static inline Err uiwrite_cb(StrView s, void* null_) {
    if (null_) return "error: unexpected context in uiwrite";
    return s.len - fwrite(s.s, 1, s.len, stdout) ? "error: fwrite failure": Ok;
}

static inline Err uiflush(void) {
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

static inline Err write_to_file(StrView s, void* f) {
    return s.len - fwrite(s.s, 1, s.len, f) ? "error: fwrite failure": Ok;
}

typedef struct { StrView s; void* ctx; } UiWriteParams;
static inline Err uiwrite_from_param(UiWriteParams p) { return uiwrite_cb(p.s, p.ctx); }

#define uiwrite_str__(S) uiwrite_cb(S,NULL)
#define uiwrite__(...) GET_MACRO__(NULL,__VA_ARGS__,uiwrite_cb,uiwrite_str__,skip__)(__VA_ARGS__)

#endif
