#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include "src/user-input.h"
#include "src/user-out.h"
#include "src/user-out-line-mode.h"
#include "src/user-out-vi-mode.h"

typedef struct Session Session;
typedef Err (*ProcessLineFn)(Session*,const char*);

typedef struct {
    UserInput     uin;
    ProcessLineFn process_line;
    UserOutput    uout;
} UserInterface ;

Err process_line_line_mode(Session* s, const char* line);

/* ctr / factories */
static inline UserInterface ui_fgets(void) {
    return (UserInterface) {
        .uin            = uinput_fgets(),
        .process_line   = process_line_line_mode,
        .uout           = uout_line_mode()
    };
}

static inline UserInterface ui_isocline(void) {
    return (UserInterface) {
        .uin            = uinput_isocline(),
        .process_line   = process_line_line_mode,
        .uout           = uout_line_mode()
    };
}

static inline UserInterface ui_vi_mode(void) {
    return (UserInterface) {
        .uin            = uinput_vi_mode(),
        .process_line   = process_line_line_mode, //todo change me :)
        .uout           = uout_vi_mode()
    };
}

/* * */

static inline void ui_switch(UserInterface old_ui[static 1], UserInterface new_ui[static 1]) {
    *old_ui = *new_ui;
}

Err ui_get_win_size(size_t nrows[static 1], size_t ncols[static 1]) ;
#endif
