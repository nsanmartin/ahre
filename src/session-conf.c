#include "session-conf.h"
#include "config.h"

Err session_conf_set_paths(SessionConf sc[_1_]) {

    Str confdir = (Str){0};
    Err err = create_or_get_confdir(&confdir);
    if (err || !confdir.len)
        return Ok; /* we ignore this error and just init a session with no config dir*/

    sc->confdirname = confdir;
    try_or_jump(err, Fail_Clean_Conf_Dir, str_append_str(&sc->cookies_fname, &sc->confdirname));
    try_or_jump(err, Fail_Clean_Cookies, append_cookies_filename(&sc->cookies_fname));

    try_or_jump(err, Fail_Clean_Cookies, str_append_str(&sc->bookmarks_fname, &sc->confdirname));
    try_or_jump(err, Fail_Clean_Bookmarks, append_bookmark_filename(NULL, &sc->bookmarks_fname));

    try_or_jump(err, Fail_Clean_Bookmarks,
        str_append_str(&sc->input_history_fname, &sc->confdirname));
    try_or_jump(err, Fail_Clean_Input_Hist, append_input_history_filename(&sc->input_history_fname));

    try_or_jump(err, Fail_Clean_Input_Hist,
        str_append_str(&sc->fetch_history_fname, &sc->confdirname));
    try_or_jump(err, Fail_Clean_Fetch_Hist, append_fetch_history_filename(&sc->fetch_history_fname));

    return err;

Fail_Clean_Fetch_Hist:
    str_clean(&sc->fetch_history_fname);
Fail_Clean_Input_Hist:
    str_clean(&sc->input_history_fname);
Fail_Clean_Bookmarks:
    str_clean(&sc->bookmarks_fname);
Fail_Clean_Cookies:
    str_clean(&sc->cookies_fname);
Fail_Clean_Conf_Dir:
    str_clean(&sc->confdirname);
    return err;
}

