#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

#include <errno.h>

#include "src/utils.h"
#include "src/user-out-vi-mode.h"
#include "src/user-out-line-mode.h"

typedef struct Session Session;

static inline Err ui_write_callback_stdout(const char* mem, size_t len, Session* s) {
    (void)s;
    return len - fwrite(mem, sizeof(const char), len, stdout) ? "error: fwrite failure": Ok;
}


static inline Err ui_flush_stdout(Session* s) {
    (void)s;
    puts("");
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

typedef struct UserOutput UserOutput;

typedef Err (*WriteUserOutputCallback)(const char* mem, size_t len, Session* ctx);
typedef Err (*FlushUserOutputCallback)(Session* s);
typedef Err (*ShowSessionUserOutputCallback)(Session* s);
typedef Err (*ShowErrUserOutputCallback)(Session* s, char* err, size_t len);
typedef void (*UserOutputCleanup)(UserOutput*);

typedef struct { WriteUserOutputCallback write; Session* ctx; } SessionWriteFn;

typedef union {
    void* void_ptr;
    ViOutCtx* vi_out_ctx;
} UserOutCtx;

struct UserOutput {
    WriteUserOutputCallback       write_msg;
    FlushUserOutputCallback       flush_msg;
    WriteUserOutputCallback       write_std;
    FlushUserOutputCallback       flush_std;
    ShowSessionUserOutputCallback show_session;
    ShowErrUserOutputCallback     show_err;
};

/* ctor / factory */
static inline UserOutput uout_line_mode(void) {
    return (UserOutput) {
        .write_msg    = ui_write_callback_stdout,
        .flush_msg    = ui_flush_stdout,
        .write_std    = ui_write_callback_stdout,
        .flush_std    = ui_flush_stdout,
        .show_session = _line_show_session_,
        .show_err     = _line_show_err_
    };
}

static inline UserOutput uout_vi_mode(void) {
    return (UserOutput) {
        .write_msg    = _vi_write_msg_,
        .flush_msg    = _vi_flush_msg_,
        .write_std    = _vi_write_std_,
        .flush_std    = _vi_flush_std_,
        .show_session = _vi_show_session_,
        .show_err     = _vi_show_err_
    };
}

/* * */

static inline Err ui_write_unsigned(WriteUserOutputCallback wcb, uintmax_t ui, Session* s) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        return "error: snprintf failure";
    }
    return wcb(numbf, len, s);
}



static inline Err ignore_output(const char* mem, size_t len, Session* s) {
    (void)mem;
    (void)len;
    (void)s;
    //puts("ignoring output :)");
    return Ok;
}

#define output_dev_null__ (SessionWriteFn){.write=ignore_output}
#define output_stdout__ (SessionWriteFn){.write=ui_write_callback_stdout}

#endif
