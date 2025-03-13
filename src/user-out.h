#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

#include <errno.h>

#include "src/utils.h"

typedef struct Session Session;

typedef Err (*WriteUserOutputCallback)(const char* mem, size_t len, void* ctx);
typedef Err (*FlushUserOutputCallback)(void);
typedef Err (*ShowTextUserOutputCallback)(Session* s);

typedef struct { WriteUserOutputCallback write; void* ctx; } WriteFnWCtx;

typedef struct {
    WriteUserOutputCallback    write_msg;
    FlushUserOutputCallback    flush_msg;
    WriteUserOutputCallback    write_std;
    FlushUserOutputCallback    flush_std;
    ShowTextUserOutputCallback show_session;
} UserOutput;


#define uiw_lit__(Cb, Lit) Cb(Lit,sizeof(Lit)-1,NULL)

static inline Err uiw_strview(WriteUserOutputCallback wcb, StrView s[static 1]) {
    return wcb(items__(s), len__(s),NULL);
}

static inline Err uiw_str(WriteUserOutputCallback wcb, Str s[static 1]) {
    return wcb(items__(s), len__(s), NULL);
}

static inline Err uiw_mem(WriteUserOutputCallback wcb, const char* mem, size_t len) {
    return wcb(mem, len,NULL);
}

static inline Err uilw_mem(WriteUserOutputCallback wcb, const char* mem, size_t len) {
    Err err = wcb(mem, len,NULL);
    ok_then(err, uiw_lit__(wcb, "\n"));
    return err;
}

static inline Err ui_write_unsigned(WriteUserOutputCallback wcb, uintmax_t ui) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        return "error: snprintf failure";
    }
    return wcb(numbf, len, NULL);
}


static inline Err ui_write_callback_stdout(const char* mem, size_t len, void* ctx) {
    if (ctx) return "error: ctx not expected in write callback";
    return len - fwrite(mem, sizeof(const char), len, stdout) ? "error: fwrite failure": Ok;
}


static inline Err ui_flush_stdout(void) {
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    return Ok;
}

static inline Err ignore_output(const char* mem, size_t len, void* ctx) {
    (void)mem;
    (void)len;
    (void)ctx;
    //puts("ignoring output :)");
    return Ok;
}

#define output_dev_null__ (WriteFnWCtx){.write=ignore_output}
#define output_stdout__ (WriteFnWCtx){.write=ui_write_callback_stdout}

#endif
