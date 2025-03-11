#ifndef AHRE_USER_OUT_LINE_MODE_H__
#define AHRE_USER_OUT_LINE_MODE_H__

#include "src/error.h"
#include "src/user-out.h"

typedef struct Session Session;

Err ui_show_session_line_mode(Session* s);

static inline UserOutput uout_line_mode(void) {
    return (UserOutput) {
        .write_msg    = ui_write_callback_stdout,
        .flush_msg    = ui_flush_stdout,
        .write_std    = ui_write_callback_stdout,
        .flush_std    = ui_flush_stdout,
        .show_session = ui_show_session_line_mode
    };
}

#endif
