#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include "src/ahre.h"
#include "src/get-options.h"
#include "src/help.h"
#include "src/htmldoc.h"
#include "src/mem.h"
#include "src/user-input.h"
#include "src/user-out.h"
#include "src/user-interface.h"
#include "src/session.h"
#include "src/reditline.h"



static Err _fetch_many_urls_(Session session[_1_], ArlOf(Request) urls[_1_]) {
    for (Request* r = arlfn(Request,begin)(urls)
        ; r != arlfn(Request,end)(urls)
        ; ++r
    ) try( session_fetch_request(session, r, session_url_client(session)));
    return Ok;
}


static Err _loop_(Session s[_1_], UserLine userln[_1_]) {
    init_user_input_history();
    Err err    = Ok;

    while (!session_quit(s)) {
        try_or_jump(err, Error, session_show_output(s));
        try_or_jump(err, Error, session_read_user_input(s, userln));
        if (!user_line_empty(userln))
            try_or_jump(err, Error, session_consume_line(s, userln));
Error:
        if (err) if (session_show_error(s, err)) return "error trying to show previous error"; 
    }
    return Ok;
}

static Err run_cmds(Session s[_1_], UserLine userln[_1_]) {
    Err err = Ok;
    Str errors = (Str){0};
    while (!user_line_empty(userln) && !session_quit(s)) {
        if (!user_line_empty(userln)) err = session_consume_line(s, userln);
        if (err) try_or_jump(err, Clean_Errors, str_append_ln(&errors, (char*)err, strlen(err)));
    }
    if (errors.len) session_write_msg(s, errors.items, errors.len);
Clean_Errors:
    str_clean(&errors);
    return err;
}


int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CliParams cparams = (CliParams){0};
    UserLine userln = (UserLine){0};
    Session session;
    Err err = Ok;

    try_or_jump(err, Clean_Curl, session_conf_from_options(argc, argv, &cparams));
    if (*cparams_help(&cparams)) { print_help(argv[0]); goto Clean_Cparams; }
    if (*cparams_version(&cparams)) { puts("ahre version " AHRE_VERSION); goto Clean_Cparams; }
    try_or_jump(err, Clean_Cparams, session_init(&session, cparams_sconf(&cparams)));
    if (cparams.cmd) {
        cparams.cmd = std_strdup(cparams.cmd);
        if (!cparams.cmd) {
            err = "error: strdup failure";
            goto Clean_Session;
        }
        try_or_jump(err, Clean_Session, user_line_init_take_ownership(&userln, cparams.cmd));
        try_or_jump(err, Clean_Session, run_cmds(&session, &userln));
        user_line_cleanup(&userln);
    }
    try_or_jump(err, Clean_Session, _fetch_many_urls_(&session, cparams_requests(&cparams)));

    err = _loop_(&session, &userln);

Clean_Session:
    session_close(&session);
    session_cleanup(&session);
Clean_Cparams:
    cparams_clean(&cparams);
Clean_Curl:
    curl_global_cleanup();

    if (err) fprintf(stderr, "ahre: %s\n", err);
    return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
