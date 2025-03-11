#ifndef AHRE_USER_OUT_VI_MODE_H__
#define AHRE_USER_OUT_VI_MODE_H__

#include "src/textbuf.h"
#include "src/ranges.h"
#include "src/user-out.h"

Err _vi_print_(TextBuf textbuf[static 1], Range range[static 1], UserOutput out[static 1]);
Err ui_show_session_vi_mode(Session* s);


static inline UserOutput uout_vi_mode(void) {
    return (UserOutput) {
        .write_msg    = ui_write_callback_stdout,
        .flush_msg    = ui_flush_stdout,
        .write_std    = ui_write_callback_stdout,
        .flush_std    = ui_flush_stdout,
        .show_session = ui_show_session_vi_mode
    };
}

#endif
