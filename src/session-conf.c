#include "session-conf.h"
#include "config.h"

Err session_conf_set_paths(SessionConf sc[_1_]) {

    Str confdir = (Str){0};
    Err err = create_or_get_confdir(&confdir);
    if (err || !confdir.len) {
        str_clean(&confdir);
        return Ok; /* we ignore this error and just init a session with no config dir*/
    }

    sc->confdirname = confdir;
    tryjmp(err, Fail, str_append(&sc->cookies_fname, &sc->confdirname));
    tryjmp(err, Fail, append_cookies_filename(&sc->cookies_fname));

    tryjmp(err, Fail, str_append(&sc->bookmarks_fname, &sc->confdirname));
    tryjmp(err, Fail, append_bookmark_filename(NULL, &sc->bookmarks_fname));

    tryjmp(err, Fail,
        str_append(&sc->input_history_fname, &sc->confdirname));
    tryjmp(err, Fail, append_input_history_filename(&sc->input_history_fname));

    tryjmp(err, Fail,
        str_append(&sc->fetch_history_fname, &sc->confdirname));
    tryjmp(err, Fail, append_fetch_history_filename(&sc->fetch_history_fname));

    return err;

Fail:
    str_clean(&sc->bookmarks_fname);
    str_clean(&sc->confdirname);
    str_clean(&sc->cookies_fname);
    str_clean(&sc->fetch_history_fname);
    str_clean(&sc->input_history_fname);
    return err;
}

