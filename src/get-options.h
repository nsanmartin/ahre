/* Copyright (C) 2026 Nicolas San Martin

This file is part of ahre.

ahre is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

ahre is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ahre; see the file LICENSE or see
<https://www.gnu.org/licenses/>.  */

#ifndef AHRE_GET_OPTIONS_H__
#define AHRE_GET_OPTIONS_H__

#include "error.h"
#include "session-conf.h"
#include "user-interface.h"
#include "utils.h"
#include "url.h"
#include "request.h"

typedef struct {
    bool             version;
    bool             help;
    SessionConf      sconf;
    ArlOf(Request)   requests;
    const char*      data;
    Str              cmd;
} CliParams;

static inline bool* cparams_version(CliParams cparams[_1_]) { return &cparams->version; }
static inline bool* cparams_help(CliParams cparams[_1_]) { return &cparams->help; }
static inline SessionConf* cparams_sconf(CliParams cparams[_1_]) { return &cparams->sconf; }
static inline ArlOf(Request)* cparams_requests(CliParams cparams[_1_]) { return &cparams->requests; }
static inline const char* cparams_data(CliParams cparams[_1_]) { return cparams->data; } 
static inline void cparams_clean(CliParams cparams[_1_]) {
    arlfn(Request,clean)(cparams_requests(cparams));
    str_clean(&cparams->cmd);
} 

Err session_conf_from_options(int argc, char* argv[], CliParams cparams[_1_]);

#endif
