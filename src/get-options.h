#ifndef AHRE_GET_OPTIONS_H__
#define AHRE_GET_OPTIONS_H__

#include "error.h"
#include "session-conf.h"
#include "user-interface.h"
#include "utils.h"

typedef struct {
    bool             version;
    bool             help;
    SessionConf      sconf;
    ArlOf(cstr_view) urls;
    const char*      data;
} CliParams;

static inline bool* cparams_version(CliParams cparams[_1_]) { return &cparams->version; }
static inline bool* cparams_help(CliParams cparams[_1_]) { return &cparams->help; }
static inline SessionConf* cparams_sconf(CliParams cparams[_1_]) { return &cparams->sconf; }
static inline ArlOf(cstr_view)*
cparams_urls(CliParams cparams[_1_]) { return &cparams->urls; }
static inline const char* cparams_data(CliParams cparams[_1_]) { return cparams->data; } 

static inline void cparams_clean(CliParams cparams[_1_]) {
    arlfn(cstr_view,clean)(cparams_urls(cparams));
} 

Err session_conf_from_options(int argc, char* argv[], CliParams cparams[_1_]);

#endif
