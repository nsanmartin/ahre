#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include "src/user-input.h"
#include "src/user-out.h"
#include "src/user-out-line-mode.h"
#include "src/user-out-vi-mode.h"

typedef struct {
    UserInput uin;
    UserOutput uout;
} UserInterface ;


Err ui_get_win_size(size_t nrows[static 1], size_t ncols[static 1]) ;

static inline UserInterface ui_fgets(void) {
    return (UserInterface) {
        .uin=uinput_fgets(),
        .uout=uout_line_mode()
    };
}

static inline UserInterface ui_isocline(void) {
    return (UserInterface) {
        .uin=uinput_isocline(),
        .uout=uout_line_mode()
    };
}

static inline UserInterface ui_vi_mode(void) {
    return (UserInterface) {
        .uin=uinput_vi_mode(),
        .uout=uout_vi_mode()
    };
}

#endif
