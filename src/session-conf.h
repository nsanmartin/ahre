#ifndef AHRE_SESSION_CONF_H__
#define AHRE_SESSION_CONF_H__

#include "src/utils.h"
#include "src/user-input.h"
#include "src/user-out.h"

#define SESSION_CONF_FLAG_QUIT       0x1u
#define SESSION_CONF_FLAG_MONOCHROME 0x2u


typedef struct {
    UserInput uin;
    size_t ncols;
    size_t nrows;
    unsigned flags;
    WriteUserOutputCallback write_msg;
    WriteUserOutputCallback write_std;
    //TODO Add flush_msg and flush_std
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
static inline UserInput* session_conf_uin(SessionConf sc[static 1]) { return &sc->uin; }
static inline size_t* session_conf_nrows(SessionConf sc[static 1]) { return &sc->nrows; }
static inline size_t* session_conf_ncols(SessionConf sc[static 1]) { return &sc->ncols; }
static inline
WriteUserOutputCallback session_conf_write_msg(SessionConf sc[static 1]) { return sc->write_msg; }
static inline
WriteUserOutputCallback session_conf_write_std(SessionConf sc[static 1]) { return sc->write_std; }

#endif
