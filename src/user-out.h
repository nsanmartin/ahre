#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

#include <errno.h>

#include "src/utils.h"

typedef struct Session Session;

typedef Err (*WriteUserOutputCallback)(const char* mem, size_t len, Session* ctx);
typedef Err (*FlushUserOutputCallback)(Session* s);
typedef Err (*ShowTextUserOutputCallback)(Session* s);

typedef struct { WriteUserOutputCallback write; Session* ctx; } SessionWriteFn;

typedef struct {
    WriteUserOutputCallback    write_msg;
    FlushUserOutputCallback    flush_msg;
    WriteUserOutputCallback    write_std;
    FlushUserOutputCallback    flush_std;
    ShowTextUserOutputCallback show_session;
} UserOutput;


#define uiw_lit__(Cb, Lit) Cb(Lit,sizeof(Lit)-1,NULL)


static inline Err ui_write_unsigned(WriteUserOutputCallback wcb, uintmax_t ui, Session* s) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        return "error: snprintf failure";
    }
    return wcb(numbf, len, s);
}


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
