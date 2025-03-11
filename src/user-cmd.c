#include "src/session.h"
#include "src/utils.h"
#include "src/user-interface.h"

/* ah cmds */

Err cmd_fetch(Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    CURLU* new_cu;
    try( url_curlu_dup(htmldoc_url(htmldoc), &new_cu));
    HtmlDoc newdoc;
    Err err = htmldoc_init_fetch_draw_from_curlu(
        &newdoc,
        new_cu,
        session_url_client(session),
        http_get,
        session_conf(session)
    );
    if (err) {
        curl_url_cleanup(new_cu);
        return err;
    }
    htmldoc_cleanup(htmldoc);
    *htmldoc = newdoc;
    return err;
}


Err cmd_write(const char* fname, Session session[static 1]) {
    TextBuf* buf;
    try( session_current_buf(session, &buf));
    if (fname && *(fname = cstr_skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            return err_fmt("append: could not open file: %s", fname);
        }
        if (fwrite(textbuf_items(buf), 1, textbuf_len(buf), fp) != textbuf_len(buf)) {
            fclose(fp);
            return err_fmt("append: error writing to file: %s", fname);
        }
        fclose(fp);
        return 0;
    }
    return "append: fname missing";
}


Err cmd_set_session_winsz(Session session[static 1]) {
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(session) = nrows;
    *session_ncols(session) = ncols;
    
    WriteUserOutputCallback w = session_uout(session)->write_msg;
    try( uiw_lit__(w, "win: nrows = ")); 
    try( ui_write_unsigned(w, nrows));
    try( uiw_lit__(w, ", ncols = "));
    try( ui_write_unsigned(w, ncols));
    try( uiw_lit__(w, "\n"));

    return Ok;
}

Err cmd_set_session_ncols(Session session[static 1], const char* line) {
    size_t ncols;
    parse_size_t_or_throw(&line, &ncols, 10);
    if (*cstr_skip_space(line)) return "invalid argument";
    *session_ncols(session) = ncols;
    return Ok;
}

Err cmd_set_session_monochrome(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    if (*line == '0') session_monochrome_set(session, false);
    else if (*line == '1') session_monochrome_set(session, true);
    else return "monochrome option should be '0' or '1'";
    return Ok;

}

Err cmd_set_session_input(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);

    UserInterface ui;
    const char* rest;
    if ((rest = csubstr_match(line, "fgets", 1)) && !*rest) ui = ui_fgets();
    else if ((rest = csubstr_match(line, "isocline", 1)) && !*rest) ui = ui_isocline();
    else if ((rest = csubstr_match(line, "vi", 1)) && !*rest) ui = ui_vi_mode();
    else return "input option should be 'getline' or 'isocline'";
    *session_ui(session) = ui;
    return Ok;

}


Err cmd_set_session(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    const char* rest;
    if ((rest=csubstr_match(line, "input", 1))) return cmd_set_session_input(session, rest);
    if ((rest=csubstr_match(line, "monochrome", 1))) return cmd_set_session_monochrome(session, rest);
    if ((rest=csubstr_match(line, "ncols", 1))) return cmd_set_session_ncols(session, rest);
    if ((rest=csubstr_match(line, "winsz", 1)) && !*rest) return cmd_set_session_winsz(session);
    return "not a session option";
}

Err cmd_set_curl(Session session[static 1], const char* line);

Err cmd_set(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    const char* rest;
    if ((rest = csubstr_match(line, "session", 1))) return cmd_set_session(session, rest);
    if ((rest = csubstr_match(line, "curl", 1))) return cmd_set_curl(session, rest);
    return "not a curl option";
}

