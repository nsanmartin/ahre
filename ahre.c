#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include "src/get-options.h"
#include "src/help.h"
#include "src/htmldoc.h"
#include "src/mem.h"
#include "src/user-input.h"
#include "src/user-out.h"
#include "src/user-interface.h"
#include "src/session.h"
#include "src/reditline.h"



static Err _open_many_urls_(Session session[static 1], ArlOf(cstr_view) urls[static 1]) {
    for (cstr_view* u = arlfn(cstr_view,begin)(urls)
        ; u != arlfn(cstr_view,end)(urls)
        ; ++u
    ) {
        Err err = session_open_url(session, *u, session->url_client);
        if (err) {
            session_write_msg(session, (char*)err, strlen(err));
            session_write_msg_lit__(session, "\n");
        }
    }
    arlfn(cstr_view,clean)(urls);
    return Ok;
}


static int _loop_(Session s[static 1]) {
    init_user_input_history();
    while (1) {
        Err err = session_show_output(s);
        char* line = NULL;
        ok_then(err, session_read_user_input(s, &line));
        if (line) ok_then(err, session_consume_line(s, line));
        if (session_quit(s)) break;
        if (err) if (session_show_error(s, err)) exit(EXIT_FAILURE); 
    }
    session_cleanup(s);
    reditline_history_cleanup();//TODO move from here
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CliParams cparams = (CliParams){0};
    Session session;

    Err err = session_conf_from_options(argc, argv, &cparams);
    if (*cparams_help(&cparams)) { print_help(argv[0]); exit(EXIT_SUCCESS); }
    if (*cparams_version(&cparams)) { puts("ahre version 0.0.0"); exit(EXIT_SUCCESS); }
    ok_then(err, session_init(&session, cparams_sconf(&cparams)));
    ok_then(err, _open_many_urls_(&session, cparams_urls(&cparams)));

    if (err) {
        fprintf(stderr, "ahre: %s\n", err);
        exit(EXIT_FAILURE); 
    }
    int rv = _loop_(&session);
    curl_global_cleanup();
    return rv;
}
