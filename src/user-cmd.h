#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include "src/ranges.h"
#include "src/user-interface.h"
#include "src/cmd-ed.h"
#include "src/wrapper-lexbor.h"

Err cmd_write(char* fname, Session session[static 1]);

Err cmd_fetch(Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    CURLU* new_cu;
    try( url_curlu_dup(htmldoc_url(htmldoc), &new_cu));
    HtmlDoc newdoc;
    Err err = htmldoc_init_fetch_browse_from_curlu(
        &newdoc,
        new_cu,
        session_url_client(session),
        http_get,
        session_monochrome(session)
    );
    if (err) {
        curl_url_cleanup(new_cu);
        return err;
    }
    htmldoc_cleanup(htmldoc);
    *htmldoc = newdoc;
    return err;
}


static inline Err cmd_tag(const char* rest, Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return lexbor_cp_tag(rest, htmldoc->lxbdoc, textbuf_buf(htmldoc_textbuf(htmldoc)));
}


static inline Err cmd_browse(Session session[static 1], const char* rest) {
    if (*rest) return "browse cmd accept no params";
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    htmldoc_reset(htmldoc);
    return htmldoc_browse(htmldoc, session_monochrome(session));
}

#endif
