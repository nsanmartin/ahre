#ifndef AHRE_SESSION_CONF_H__
#define AHRE_SESSION_CONF_H__

#include "utils.h"
#include "user-interface.h"


#define SESSION_CONF_FLAG_QUIT       0x1u
#define SESSION_CONF_FLAG_MONOCHROME 0x2u
#define SESSION_CONF_FLAG_JS         0x4u
#define SESSION_CONF_SHOW_FORM_JS    0x8u

typedef struct {
    UserInterface ui;
    size_t        ncols;
    size_t        nrows;
    unsigned      flags;
    Str           confdirname;
    Str           bookmarks_fname;
    Str           cookies_fname;
    Str           input_history_fname;
    Str           fetch_history_fname;
} SessionConf ;

static inline
bool session_conf_quit(SessionConf sc[_1_]) { return sc->flags & SESSION_CONF_FLAG_QUIT; }

static inline void
session_conf_quit_set(SessionConf sc[_1_]) { sc->flags |= SESSION_CONF_FLAG_QUIT; }

static inline bool
session_conf_monochrome(SessionConf sc[_1_]) {return sc->flags & SESSION_CONF_FLAG_MONOCHROME;}

static inline bool
session_conf_js(SessionConf sc[_1_]) {return sc->flags & SESSION_CONF_FLAG_JS;}

static inline bool session_conf_show_forms(SessionConf sc[_1_]) {
    return sc->flags & SESSION_CONF_SHOW_FORM_JS;
}


static inline void session_conf_show_forms_set(SessionConf sc[_1_], bool value) {
    set_flag(&sc->flags, SESSION_CONF_SHOW_FORM_JS, value);
}

static inline void session_conf_monochrome_set(SessionConf sc[_1_], bool value) {
    set_flag(&sc->flags, SESSION_CONF_FLAG_MONOCHROME, value);
}
static inline void session_conf_js_set(SessionConf sc[_1_], bool value) {
    set_flag(&sc->flags, SESSION_CONF_FLAG_JS, value);
}
static inline size_t* session_conf_nrows(SessionConf sc[_1_]) { return &sc->nrows; }
static inline size_t* session_conf_ncols(SessionConf sc[_1_]) { return &sc->ncols; }

static inline UserInterface* session_conf_ui(SessionConf sc[_1_]) { return &sc->ui; }
static inline UserInput* session_conf_uinput(SessionConf sc[_1_]) { return &sc->ui.uin; }
static inline UserOutput* session_conf_uout(SessionConf sc[_1_]) { return &sc->ui.uout; }

static inline void session_conf_cleanup(SessionConf sc[_1_]) {
    str_clean(&sc->confdirname);
    str_clean(&sc->bookmarks_fname);
    str_clean(&sc->cookies_fname);
    str_clean(&sc->input_history_fname);
    str_clean(&sc->fetch_history_fname);
}
Err session_conf_set_paths(SessionConf sc[_1_]);
#endif
