#ifndef AHRE_GET_OPTIONS_H__
#define AHRE_GET_OPTIONS_H__

#include "src/error.h"
#include "src/session-conf.h"
#include "src/user-interface.h"
#include "src/utils.h"

typedef struct {
    bool version;
    bool help;
    SessionConf sconf;
    ArlOf(const_char_ptr) urls;
} CliParams;

static inline bool* cparams_version(CliParams cparams[static 1]) { return &cparams->version; }
static inline bool* cparams_help(CliParams cparams[static 1]) { return &cparams->help; }
static inline SessionConf* cparams_sconf(CliParams cparams[static 1]) { return &cparams->sconf; }
static inline ArlOf(const_char_ptr)*
cparams_urls(CliParams cparams[static 1]) { return &cparams->urls; }


Err session_conf_from_options(int argc, char* argv[], CliParams cparams[static 1]);

#endif
