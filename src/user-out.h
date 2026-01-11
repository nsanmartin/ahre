#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

#include <errno.h>

#include "utils.h"
#include <curl/curl.h>
#include "cmd-out.h"


typedef struct Session Session;
typedef struct UserOutput UserOutput;

typedef Err (*WriteUserOutputCallback)(const char* mem, size_t len, Session* ctx);
typedef Err (*FlushUserOutputCallback)(Session* s, CmdOut cout[_1_]);
typedef Err (*ShowSessionUserOutputCallback)(Session* s, CmdOut cout[_1_]);
typedef Err (*ShowErrUserOutputCallback)(Session* s, char* err, size_t len);
typedef void (*UserOutputCleanup)(UserOutput*);

typedef struct { WriteUserOutputCallback write; Session* ctx; } SessionWriteFn;


struct UserOutput {
    FlushUserOutputCallback       flush_msg;
    FlushUserOutputCallback       flush_std;
    ShowSessionUserOutputCallback show_session;
    ShowErrUserOutputCallback     show_err;
};

/*
 * line mode
 */

static inline Err ui_write_callback_stdout(const char* mem, size_t len, Session* s) {
    (void)s;
    return len - fwrite(mem, sizeof(const char), len, stdout) ? "error: fwrite failure": Ok;
}


static inline Err ui_line_flush_stdout(Session* s, CmdOut cout[_1_]) {
    (void)cout;
    (void)s;
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

Err ui_line_show_session(Session* s, CmdOut cout[_1_]);
Err ui_line_show_err(Session* s, char* err, size_t len);
//
// line mode


/*
 * visual mode
 */

typedef struct Session Session;
Err ui_vi_show_session(Session* s, CmdOut cout[_1_]);
Err ui_vi_write_msg(const char* mem, size_t len, Session* s);
Err ui_vi_flush_msg(Session* s, CmdOut cout[_1_]);
Err ui_vi_show_err(Session* s, char* err, size_t len);
Err ui_vi_flush_std(Session* s, CmdOut cout[_1_]);

/* getter */

void _vi_cleanup_(UserOutput* uo);

// visual mode
//

/* ctor / factory */
static inline UserOutput uout_line_mode(void) {
    return (UserOutput) {
        .flush_msg    = ui_line_flush_stdout,
        .flush_std    = ui_line_flush_stdout,
        .show_session = ui_line_show_session,
        .show_err     = ui_line_show_err,
    };
}

static inline UserOutput uout_vi_mode(void) {
    return (UserOutput) {
        .flush_msg    = ui_vi_flush_msg,
        .flush_std    = ui_vi_flush_std,
        .show_session = ui_vi_show_session,
        .show_err     = ui_vi_show_err
    };
}

/* * */



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
