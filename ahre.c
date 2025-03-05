#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include "src/get_options.h"
#include "src/help.h"
#include "src/htmldoc.h"
#include "src/mem.h"
#include "src/user-input.h"
#include "src/user-out.h"
#include "src/user-interface.h"


static Err _open_many_urls_(Session session[static 1], ArlOf(const_char_ptr) urls[static 1]) {
    for (const_char_ptr* u = arlfn(const_char_ptr,begin)(urls)
        ; u != arlfn(const_char_ptr,end)(urls)
        ; ++u
    ) {
        Err err = session_open_url(session, *u, session->url_client);
        if (err) puts(err);
    }
    arlfn(const_char_ptr,clean)(urls);
    return Ok;
}

static int _loop_(Session session[static 1]) {
    Err err;
    init_user_input_history();
    while (!session_quit(session)) {
        if ((err = read_line_from_user(session))) {
            WriteUserOutputCallback w = session_uout(session)->write_msg;
            Err suberr = uiw_mem(w, err, strlen(err));
            ok_then(suberr, uiw_lit__(w, "\n"));
            if (suberr) { /* what else could I do */
                fprintf(stderr, "a real error: %s\n", suberr);
            }
        }
    }
    session_cleanup(session);
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    CliParams cparams = (CliParams){0};
    Session session;

    Err err = session_conf_from_options(argc, argv, &cparams);
    if (*cparams_help(&cparams)) { print_help(argv[0]); exit(EXIT_SUCCESS); }
    if (*cparams_version(&cparams)) { puts("ahre version 0.0.0"); exit(EXIT_SUCCESS); }
    ok_then(err, session_init(&session, cparams_sconf(&cparams)));
    ok_then(err, _open_many_urls_(&session, cparams_urls(&cparams)));

    if (!err) return _loop_(&session);

    fprintf(stderr, "ahre: %s\n", err);
    exit(EXIT_FAILURE); 
}
