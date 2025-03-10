#ifndef AHRE_USER_OUT_MODES_H__
#define AHRE_USER_OUT_MODES_H__

/* Vi Mode */




static inline UserOutput uout_vi_mode_out(void) {
    return (UserOutput) {
        .write_msg = ui_write_callback_stdout,
        .flush_msg = ui_flush_stdout,
        .write_std = ui_write_callback_stdout,
        .flush_std = ui_flush_stdout
    };
}

#endif
