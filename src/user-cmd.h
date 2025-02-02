#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include "src/ranges.h"
#include "src/user-interface.h"
#include "src/cmd-ed.h"
#include "src/wrapper-lexbor.h"

Err cmd_write(char* fname, Session session[static 1]);
//Err cmd_set_url(Session session[static 1], const char* url);

Err cmd_fetch(Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return htmldoc_fetch(htmldoc, session->url_client);
}


//TODO: use reset
///static inline Err cmd_clear(Session session[static 1]) {
///    textbuf_cleanup(session_current_buf(session));
///    return Ok;
///}

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
    return htmldoc_browse(htmldoc);
}

#endif
