#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include "src/ranges.h"
#include "src/user-interface.h"
#include "src/cmd-ed.h"
#include "src/wrapper-lexbor.h"

Err cmd_write(char* fname, Session session[static 1]);

Err cmd_fetch(Session session[static 1]) {
    HtmlDoc* html_doc = session_current_doc(session);
    if (html_doc->url) {
        return doc_fetch(html_doc, session->url_client);
    }
    return "Not url to fech";
}


static inline Err cmd_ahre(Session session[static 1]) {
    HtmlDoc* html_doc = session_current_doc(session);
    TextBuf* tb = &html_doc->textbuf;
    Err err = lexbor_href_write(html_doc->lxbdoc, &html_doc->cache.hrefs, tb);
    if (err) return err; 
    
    return textbuf_append_null(tb);
}

static inline Err cmd_clear(Session session[static 1]) {
    textbuf_cleanup(session_current_buf(session));
    return Ok;
}

static inline Err cmd_tag(const char* rest, Session session[static 1]) {
    HtmlDoc* html_doc = session_current_doc(session);
    return lexbor_cp_tag(rest, html_doc->lxbdoc, &html_doc->textbuf.buf);
}

static inline Err cmd_text(Session* session) {
    HtmlDoc* html_doc = session_current_doc(session);
    return lexbor_html_text_append(html_doc->lxbdoc, &html_doc->textbuf);
}


#endif
