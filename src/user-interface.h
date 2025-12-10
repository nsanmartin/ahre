#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include "user-input.h"
#include "user-out.h"

typedef struct Session Session;
typedef Err (*ProcessLineFn)(Session*,const char*);

typedef struct {
    UserInput     uin;
    //TODO: is this needed?
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
        .process_line   = process_line_line_mode,
        .uout           = uout_vi_mode()
    };
}

/* * */

static inline void ui_switch(UserInterface old_ui[_1_], UserInterface new_ui[_1_]) {
    *old_ui = *new_ui;
}

Err ui_get_win_size(size_t nrows[_1_], size_t ncols[_1_]) ;
#endif
