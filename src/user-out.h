#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

#include <errno.h>

#include "utils.h"

typedef struct Session Session;
typedef struct UserOutput UserOutput;

typedef Err (*WriteUserOutputCallback)(const char* mem, size_t len, Session* ctx);
typedef Err (*FlushUserOutputCallback)(Session* s);
typedef Err (*ShowSessionUserOutputCallback)(Session* s);
typedef Err (*ShowErrUserOutputCallback)(Session* s, char* err, size_t len);
typedef void (*UserOutputCleanup)(UserOutput*);

typedef struct { WriteUserOutputCallback write; Session* ctx; } SessionWriteFn;

struct UserOutput {
    WriteUserOutputCallback       write_msg;
    FlushUserOutputCallback       flush_msg;
    WriteUserOutputCallback       write_std;
    FlushUserOutputCallback       flush_std;
    ShowSessionUserOutputCallback show_session;
    ShowErrUserOutputCallback     show_err;
};

static inline Err ui_write_callback_stdout(const char* mem, size_t len, Session* s) {
    (void)s;
    return len - fwrite(mem, sizeof(const char), len, stdout) ? "error: fwrite failure": Ok;
}

/* line mode */
static inline Err ui_line_flush_stdout(Session* s) {
    (void)s;
    fwrite("\n", 1, 1, stdout);
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

Err ui_line_show_session(Session* s);
Err ui_line_show_err(Session* s, char* err, size_t len);
/***/
/* vi mode */
typedef struct Session Session;
Err ui_vi_show_session(Session* s);
Err ui_vi_write_msg(const char* mem, size_t len, Session* s);
Err ui_vi_flush_msg(Session* s);
Err ui_vi_show_err(Session* s, char* err, size_t len);
Err ui_vi_flush_std(Session* s);
Err ui_vi_write_std(const char* mem, size_t len, Session* s);

/* getter */

void _vi_cleanup_(UserOutput* uo);
/***/

/* ctor / factory */
static inline UserOutput uout_line_mode(void) {
    return (UserOutput) {
        .write_msg    = ui_write_callback_stdout,
        .flush_msg    = ui_line_flush_stdout,
        .write_std    = ui_write_callback_stdout,
        .flush_std    = ui_line_flush_stdout,
        .show_session = ui_line_show_session,
        .show_err     = ui_line_show_err
    };
}

static inline UserOutput uout_vi_mode(void) {
    return (UserOutput) {
        .write_msg    = ui_vi_write_msg,
        .flush_msg    = ui_vi_flush_msg,
        .write_std    = ui_vi_write_std,
        .flush_std    = ui_vi_flush_std,
        .show_session = ui_vi_show_session,
        .show_err     = ui_vi_show_err
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
