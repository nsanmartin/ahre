#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include "ranges.h"
#include "user-interface.h"
#include "cmd-ed.h"
#include "wrapper-lexbor.h"

Err cmd_write(char* fname, Session session[static 1]);

Err cmd_fetch(Session session[static 1]);


static inline Err cmd_tag(const char* rest, Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return lexbor_cp_tag(rest, htmldoc->lxbdoc, textbuf_buf(htmldoc_textbuf(htmldoc)));
}

Err cmd_draw(Session session[static 1], const char* rest);

#endif
