#include <stdbool.h>
#include <stdio.h>
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
#include "wrapper-curl.h"
#include "generic.h"

Err main_ahre(int argc, char **argv);

static void warn_cmd_out_to_stderr(CmdOut cout[_1_]) {
    Msg*    msg  = cmd_out_msg(cout);
    if (!len__(msg_str(msg))) return;
    StrView warn = svl("ahre warn: ");
    mem_fwrite(warn.items, warn.len, stderr);
    mem_fwrite(msg_items(msg), msg_len(msg), stderr);
    fflush(stderr);
    msg_clean(msg);
}


static Err fetch_params(Session session[_1_], ArlOf(Request) urls[_1_], CmdOut cout[_1_]) {
    foreach__(Request,r,urls) {
        Err err = session_fetch_request(session, r, session_url_client(session), cout);
        if (err) {
            Str     buf = (Str){0};
            StrView url = sv(r->urlstr);

            if (url.len) {
                Err append_err = str_append_z(&buf, sv(err));
                if (!append_err) err = err_fmt("ahre (processing url '%s'): %s", url.items, buf.items);

                request_clean(r);
                str_clean(&buf);
            }
            return err;
        }
    }
    return Ok;
}


static Err _loop_(Session s[_1_], UserLine userln[_1_], CmdOut cout[_1_]) {
    init_user_input_history();
    Err err      = Ok;

    while (!session_quit(s)) {
        try_or_jump(err, Error, session_show_output(s, cout));
        try_or_jump(err, Error, session_read_user_input(s, userln));
        if (!user_line_empty(userln))
            try_or_jump(err, Error, session_consume_line(s, userln, cout));
Error:
        if (err) if (session_show_error(s, err)) return "error trying to show previous error"; 
    }
    return Ok;
}

static Err run_cmds(Session s[_1_], UserLine userln[_1_]) {
    Err err      = Ok;
    Str errors   = (Str){0};
    CmdOut* cout = &(CmdOut){0};
    while (!user_line_empty(userln) && !session_quit(s)) {
        if (!user_line_empty(userln)) err = session_consume_line(s, userln, cout);
        if (err) try_or_jump(err, Clean_Errors, str_append_ln(&errors, err));
    }
    if (errors.len) cmd_out_msg_append(cout, errors);
Clean_Errors:
    str_clean(&errors);
    cmd_out_clean(cout);
    return err;
}


Err main_ahre(int argc, char **argv) {
    w_curl_global_init();

    Err       err     = Ok;
    CmdOut*   cout    = &(CmdOut){0};
    UserLine  userln  = (UserLine){0};
    CliParams cparams = (CliParams){0};
    Session   session = (Session){0};

    try_or_jump(err, Clean, session_conf_from_options(argc, argv, &cparams));
    if (*cparams_help(&cparams)) { print_help(argv[0]); goto Clean; }
    if (*cparams_version(&cparams)) { puts("ahre version " AHRE_VERSION); goto Clean; }
    try_or_jump(err, Clean, session_init(&session, cparams_sconf(&cparams)));
    if (cparams.cmd) {
        cparams.cmd = std_strdup(cparams.cmd);
        if (!cparams.cmd) {
            err = "error: strdup failure";
            goto Clean;
        }
        try_or_jump(err, Clean, user_line_init_take_ownership(&userln, cparams.cmd));
        try_or_jump(err, Clean, run_cmds(&session, &userln));
        user_line_cleanup(&userln);
    }
    try_or_jump(err, Clean, fetch_params(&session, cparams_requests(&cparams), cout));

    err = _loop_(&session, &userln, cout);
Clean:
    warn_cmd_out_to_stderr(cout);
    cmd_out_clean(cout);
    session_close(&session);
    session_cleanup(&session);
    cparams_clean(&cparams);
    w_curl_global_cleanup();
    return err;
}

int main(int argc, char **argv) {
    Err err = main_ahre(argc, argv);
    if (err) {
        fprintf(stderr, "%s\n", err);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
