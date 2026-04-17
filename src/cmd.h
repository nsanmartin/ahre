#ifndef __AHRE_CMD_H__
#define __AHRE_CMD_H__

#include "draw.h"
#include "error.h"
#include "session.h"
#include "cmd-params.h"
#include "bookmark.h"

#define cmd_assert_no_params(Ln) do{ if(*Ln) return "error: expecting no params"; }while(0)


typedef Err (*SessionCmdFn)(CmdParams p[_1_]);
typedef struct SessionCmd SessionCmd;
typedef struct SessionCmd {
    const char* name;
    size_t len;
    size_t match;
    unsigned flags;
    const char* help;
    SessionCmdFn fn;
    SessionCmd* subcmds;
} SessionCmd ;

/*
 * Session commands
 */

#define CMD_GET_DOC \
    "Open the given url as a new tab.\n"\
    "If the url is an existing file in the host ahre will try to open it,\n"\
    "if it is an 'alias' (such as \\bookmark) it will open it.\n"\
    "Otherwise, it will curl it.\n"
Err cmd_get(CmdParams p[_1_]);

#define CMD_POST_DOC \
    "Open the given url as a new tab, using POST method\n"\
    "If the url is an existing file in the host ahre will try to open it,\n"\
    "if it is an 'alias' (such as \\bookmark) it will open it.\n"\
    "Otherwise, it will curl it.\n"

Err cmd_post(CmdParams p[_1_]);
Err cmd_set_session_bookmark(CmdParams p[_1_]);
Err cmd_set_session_input(CmdParams p[_1_]);
Err cmd_set_session_js(CmdParams p[_1_]);
Err cmd_set_session_monochrome(CmdParams p[_1_]);
Err cmd_set_session_ncols(CmdParams p[_1_]);
Err cmd_set_session_winsz(CmdParams p[_1_]);
Err shortcut_z(Session session[_1_], const char* rest, CmdOut cmd_out[_1_]);

/*
 * Curl commands
 */

#define CMD_CURL_COOKIES_DOC \
    "Show the cookies.\n"
Err cmd_curl_cookies(CmdParams p[_1_]);

#define CMD_CURL_VERSION_DOC \
    "Shows curl's version.\n"
Err cmd_curl_version(CmdParams p[_1_]);

// curl commands


/*
 *  Doc commands
 */

#define CMD_DOC_DRAW \
    "Redraws the current document.\n"
Err cmd_doc_draw(CmdParams p[_1_]);

#define CMD_DOC_JS \
    "Switch js engine for doc.\n"
Err cmd_doc_js(CmdParams p[_1_]);

#define CMD_DOC_CONSOLE \
    "js engine console\n"
Err cmd_doc_console(CmdParams p[_1_]);


/*
 * Tabs commands
 */

Err cmd_tabs(CmdParams p[_1_]);
Err cmd_tabs_info(CmdParams p[_1_]);
Err cmd_tabs_back(CmdParams p[_1_]);
Err cmd_tabs_goto(CmdParams p[_1_]);

/* 
 * HtmlDoc commands
 */
Err cmd_doc(CmdParams p[_1_]);

#define CMD_DOC_INFO_DOC \
    "Prints document information such as the title and its url.\n"
Err cmd_doc_info(CmdParams p[_1_]);
Err cmd_doc_A(CmdParams p[_1_]);

#define CMD_DOC_BOOKMARK_ADD \
    ".+[/]SECTION_NAME\n\n"\
    "Adds current document to bookmarks.\n\n"\
    "If section name is preceeded by '/', te substring given is used to\n"\
    "match a section name in the bookmark.\n"\
    "If not, the section is created unless there is a complete match in which \n"\
    "case the document is added to it.\n"
Err cmd_doc_bookmark_add(CmdParams p[_1_]);


/*
 * TextBuf commands
 */


typedef Err (*TextBufCmdFn)
    (Session s[_1_], TextBuf tb[_1_], Range r[_1_], const char* ln);


Err _cmd_textbuf_write_impl(TextBuf textbuf[_1_], Range r[_1_], const char* rest, CmdOut* out);
Err _textbuf_print_n_(TextBuf textbuf[_1_], Range range[_1_], const char* ln, CmdOut* out);
Err cmd_textbuf_global(CmdParams p[_1_]);
Err cmd_textbuf_print(CmdParams p[_1_]);
Err cmd_textbuf_print_n(CmdParams p[_1_]);
Err cmd_textbuf_write(CmdParams p[_1_]);


/*
 * Anchor commands
 */

Err _cmd_anchor_asterisk(Session session[_1_], size_t linknum, CmdOut* out);
Err _cmd_anchor_print(CmdParams p[_1_], size_t linknum);
Err _cmd_anchor_save(Session session[_1_], size_t ix, const char* fname, CmdOut* out);
Err _cmd_anchor_range_save_to_dir(
    Session session[_1_], Range r[_1_], const char* dirname, CmdOut* out
);


/*
 * Input commands
 */
Err _cmd_input_ix_set_(CmdParams p[_1_], const size_t ix);



Err _cmd_lexbor_node_print_(ArlOf(DomNode) node_arl[_1_], size_t ix, CmdOut out[_1_]);

Err _cmd_form_print(CmdParams p[_1_], size_t ix);
Err cmd_input_print(CmdParams p[_1_], size_t ix);
Err _cmd_input_submit_ix(CmdParams p[_1_], size_t ix);
Err cmd_input_default_ix(CmdParams p[_1_], size_t ix);


/*
  Image Commands
*/

Err cmd_image(CmdParams p[_1_]);
Err _cmd_image_print(CmdParams p[_1_], size_t ix);
Err _cmd_image_save(CmdParams p[_1_], size_t ix);


/*
   Misc commands
 */

// Err _cmd_misc(Session session[_1_], const char* line, CmdOut cout[_1_]);

// static inline Err _cmd_misc_tag(const char* rest, Session session[_1_]) {
//     HtmlDoc* htmldoc;
//     try( session_current_doc(session, &htmldoc));
//     return lexbor_cp_tag(rest, htmldoc->lxbdoc, textbuf_buf(htmldoc_textbuf(htmldoc)));
// }

Err cmd_fetch(Session session[_1_], CmdOut* out);
Err cmd_set_session_forms(CmdParams p[_1_]);
#endif
