#ifndef AHRE_SESSION_CONF_H__
#define AHRE_SESSION_CONF_H__

#include "src/utils.h"
#include "src/user-interface.h"


#define SESSION_CONF_FLAG_QUIT       0x1u
#define SESSION_CONF_FLAG_MONOCHROME 0x2u

typedef struct {
    UserInterface ui;
    size_t ncols;
    size_t nrows;
    unsigned flags;
} SessionConf ;

static inline
bool session_conf_quit(SessionConf sc[static 1]) { return sc->flags & SESSION_CONF_FLAG_QUIT; }

static inline void
session_conf_quit_set(SessionConf sc[static 1]) { sc->flags |= SESSION_CONF_FLAG_QUIT; }

static inline bool
session_conf_monochrome(SessionConf sc[static 1]) {return sc->flags & SESSION_CONF_FLAG_MONOCHROME;}

static inline void session_conf_monochrome_set(SessionConf sc[static 1], bool value) {
    if (value) sc->flags |= SESSION_CONF_FLAG_MONOCHROME;
    else sc->flags &= ~SESSION_CONF_FLAG_MONOCHROME;
}
static inline size_t* session_conf_nrows(SessionConf sc[static 1]) { return &sc->nrows; }
static inline size_t* session_conf_ncols(SessionConf sc[static 1]) { return &sc->ncols; }

static inline UserInterface* session_conf_ui(SessionConf sc[static 1]) { return &sc->ui; }
static inline UserInput* session_conf_uinput(SessionConf sc[static 1]) { return &sc->ui.uin; }
static inline UserOutput* session_conf_uout(SessionConf sc[static 1]) { return &sc->ui.uout; }

#endif
