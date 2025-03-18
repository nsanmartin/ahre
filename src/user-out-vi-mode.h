#ifndef AHRE_USER_OUT_VI_MODE_H__
#define AHRE_USER_OUT_VI_MODE_H__

#include "src/textbuf.h"
#include "src/ranges.h"
#include "src/user-out.h"

Err _vi_show_session_(Session* s);
Err _vi_write_msg_(const char* mem, size_t len, Session* s);
Err _vi_flush_msg_(Session* s);
Err _vi_show_err_(Session* s, char* err, size_t len);

static inline UserOutput uout_vi_mode(void) {
    return (UserOutput) {
        //.write_msg    = ui_write_callback_stdout,
        .write_msg    = _vi_write_msg_,
        .flush_msg    = _vi_flush_msg_,
        .write_std    = ui_write_callback_stdout,
        .flush_std    = ui_flush_stdout,
        .show_session = _vi_show_session_,
        .show_err     = _vi_show_err_
    };
}

#endif
