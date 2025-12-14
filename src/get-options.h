#ifndef AHRE_GET_OPTIONS_H__
#define AHRE_GET_OPTIONS_H__

#include "error.h"
#include "session-conf.h"
#include "user-interface.h"
#include "utils.h"
#include "url.h"

#define T Request
#include <arl.h>

typedef struct {
    bool             version;
    bool             help;
    SessionConf      sconf;
    ArlOf(Request)   requests;
    const char*      data;
    const char*      cmd;
} CliParams;

static inline bool* cparams_version(CliParams cparams[_1_]) { return &cparams->version; }
static inline bool* cparams_help(CliParams cparams[_1_]) { return &cparams->help; }
static inline SessionConf* cparams_sconf(CliParams cparams[_1_]) { return &cparams->sconf; }

static inline ArlOf(Request)*
cparams_requests(CliParams cparams[_1_]) { return &cparams->requests; }

static inline const char* cparams_data(CliParams cparams[_1_]) { return cparams->data; } 

static inline void cparams_clean(CliParams cparams[_1_]) {
    arlfn(Request,clean)(cparams_requests(cparams));
} 

Err session_conf_from_options(int argc, char* argv[], CliParams cparams[_1_]);

#endif
