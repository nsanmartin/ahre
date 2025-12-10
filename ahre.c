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



static Err _open_many_urls_(Session session[_1_], ArlOf(cstr_view) urls[_1_], const char* data) {
    for (cstr_view* u = arlfn(cstr_view,begin)(urls)
        ; u != arlfn(cstr_view,end)(urls)
        ; ++u
    ) {
        Request r = (Request){0};
        r.method     = data ? http_post : http_get;
        r.url        = (Str){.items=(char*)*u, .len=strlen(*u)};
        if (data) r.postfields = (Str){.items=(char*)data, .len=strlen(data)};
        Err e = session_fetch_request(session, &r, session_url_client(session));
        if (e) session_write_msg_ln(session, (char*)e, strlen(e));
    }
    return Ok;
}


static int _loop_(Session s[_1_]) {
    init_user_input_history();
    while (1) {
        Err err = session_show_output(s);
        char* line = NULL;
        ok_then(err, session_read_user_input(s, &line));
        if (line) ok_then(err, session_consume_line(s, line));
        if (session_quit(s)) break;
        if (err) if (session_show_error(s, err)) exit(EXIT_FAILURE); 
    }
    session_close(s);
    session_cleanup(s);
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CliParams cparams = (CliParams){0};
    Session session;

    Err err = session_conf_from_options(argc, argv, &cparams);
    if (*cparams_help(&cparams)) { print_help(argv[0]); exit(EXIT_SUCCESS); }
    if (*cparams_version(&cparams)) { puts("ahre version " AHRE_VERSION); exit(EXIT_SUCCESS); }
    if (len__(cparams_urls(&cparams)) > 1 && cparams_data(&cparams))
        err = "If --data|-d i s passed then only one url is supported";
    ok_then(err, session_init(&session, cparams_sconf(&cparams)));
    ok_then(err, _open_many_urls_(&session, cparams_urls(&cparams), cparams_data(&cparams)));

    int rv;
    if (err) {
        fprintf(stderr, "ahre: %s\n", err);
        rv = EXIT_FAILURE; 
    } else rv = _loop_(&session);

    cparams_clean(&cparams);
    curl_global_cleanup();
    return rv;
}
