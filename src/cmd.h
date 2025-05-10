#ifndef __AHRE_CMD_H__
#define __AHRE_CMD_H__

#include "error.h"
#include "session.h"
#include "ahre__.h"
#include "draw.h"

#define cmd_assert_no_params(Ln) do{ if(*Ln) return "error: expecting no params"; }while(0)
Err _cmd_parse_range(
    Range range[static 1], const char* inputline[static 1], TextBuf textbuf[static 1]
);

typedef enum { cmd_base_tag, cmd_textbuf_tag } CmdTag;
typedef struct { const char* ln; Session* s; TextBuf* tb; Range r; size_t ix; } CmdParams;
typedef Err (*SessionCmdFn)(CmdParams p[static 1]);
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

#define CMD_GO_DOC \
    "Open the given url as a new tab.\n"\
    "If the url is an existing file in the host ahre will try to open it,\n"\
    "if it is an 'alias' (such as \\bookmark) it will open it.\n"\
    "Otherwise, it will curl it.\n"
Err cmd_go(CmdParams p[static 1]);

#define CMD_COOKIES_DOC \
    "Show the cookies.\n"
static inline Err cmd_cookies(CmdParams p[static 1]) {
    return url_client_print_cookies(session_url_client(p->s));
}

static inline Err session_doc_draw(Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    htmldoc_reset_draw(htmldoc);
    unsigned flags = (session_monochrome(session)? DRAW_CTX_FLAG_MONOCHROME: 0);
    return htmldoc_draw_with_flags(htmldoc, session, flags);
}

#define CMD_DOC_DRAW \
    "Redraws the current document.\n"
static inline Err cmd_doc_draw(CmdParams p[static 1]) { return session_doc_draw(p->s); }

/*
 * Tabs commands
 */

Err cmd_tabs(CmdParams p[static 1]);

static inline Err cmd_tabs_info(CmdParams p[static 1]) {
    cmd_assert_no_params(p->ln);
    TabList* f = session_tablist(p->s);
    return tablist_info(p->s, f);
}

static inline Err cmd_tabs_back(CmdParams p[static 1]) {
    cmd_assert_no_params(p->ln);
    TabList* f = session_tablist(p->s);
    return tablist_back(f);
}

static inline Err cmd_tabs_goto(CmdParams p[static 1]) {
    TabList* f = session_tablist(p->s);
    return tablist_move_to_node(f, p->ln);
}

/* 
 * HtmlDoc commands
 */
Err _cmd_doc_dbg_traversal(Session ctx[static 1], const char* f);
Err cmd_doc(CmdParams p[static 1]);

#define CMD_DOC_INFO_DOC \
    "Prints document information such as the title and its url.\n"
static inline Err cmd_doc_info(CmdParams p[static 1]) {
    cmd_assert_no_params(p->ln);
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return htmldoc_print_info(p->s, d);
}

static inline Err cmd_doc_A(CmdParams p[static 1]) {
    cmd_assert_no_params(p->ln);
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return htmldoc_A(p->s, d);
}

Err bookmark_add_to_section(Session s[static 1], const char* line, UrlClient url_client[static 1]);
#define CMD_DOC_BOOKMARK_ADD \
    ".+[/]SECTION_NAME\n\n"\
    "Adds current document to bookmarks.\n\n"\
    "If section name is preceeded by '/', te substring given is used to\n"\
    "match a section name in the bookmark.\n"\
    "If not, the section is created unless there is a complete match in which \n"\
    "case the document is added to it.\n"
static inline Err cmd_doc_bookmark_add(CmdParams p[static 1]) {
    return bookmark_add_to_section(p->s, p->ln, session_url_client(p->s));
}

static inline Err _cmd_doc_hide(Session session[static 1], const char* tags) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) |= (~ts); }
    return err; 
}

static inline Err _cmd_doc_show(Session session[static 1], const char* tags) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) &= (~ts); }
    return err; 
}


/*
 * TextBuf commands
 */

Err cmd_textbuf_global(CmdParams p[static 1]);
StrView parse_pattern(const char* tk);


//TODO: pass session to this fn
//static inline Err _cmd_textbuf_print_last_range(Session s[static 1], TextBuf textbuf[static 1]) {
//    Range r = *textbuf_last_range(textbuf);
//    try( session_write_msg_lit__(s, "last range: ("));
//    try( session_write_unsigned_msg(s, r.beg));
//    try( session_write_msg_lit__(s, ", "));
//    try( session_write_unsigned_msg(s, r.end));
//    try( session_write_msg_lit__(s, ")\n"));
//    return Ok;
//}

//static inline Err _cmd_textbuf_print_all(TextBuf textbuf[static 1]) {
//    BufOf(char)* buf = &textbuf->buf;
//    fwrite(buf->items, 1, buf->len, stdout);
//    return NULL;
//}

typedef Err (*TextBufCmdFn)
    (Session s[static 1], TextBuf tb[static 1], Range r[static 1], const char* ln);

static inline Err _run_textbuf_cmd_(Session s[static 1], const char* line, TextBufCmdFn fn) {
    Range range;
    TextBuf* textbuf;
    try( session_current_buf(s, &textbuf));
    try( _cmd_parse_range(&range, &line, textbuf));
    return fn(s, textbuf, &range, line);
}


static inline Err cmd_textbuf_print(CmdParams p[static 1]) {
    SessionMemWriter writer = (SessionMemWriter){.s=p->s, .write=session_writer_write_std};
    return session_write_range_mod(&writer, p->tb, &p->r);
}


Err dbg_print_all_lines_nums(
    Session s[static 1], TextBuf tb[static 1], Range r[static 1], const char* ln);

static inline Err
cmd_textbuf_dbg_print_all_lines_nums(CmdParams p[static 1]) {
    return dbg_print_all_lines_nums(p->s, p->tb, &p->r, p->ln);
}


Err _textbuf_print_n_(
    Session s[static 1], TextBuf textbuf[static 1], Range range[static 1], const char* ln);

static inline Err
cmd_textbuf_print_n(CmdParams p[static 1]) { return  _textbuf_print_n_(p->s, p->tb, &p->r, p->ln); }

Err _cmd_textbuf_write_impl(
    Session s[static 1], TextBuf textbuf[static 1], Range r[static 1], const char* rest);

static inline Err cmd_textbuf_write(CmdParams p[static 1]) {
    return _cmd_textbuf_write_impl(p->s, p->tb, &p->r, p->ln);
}

/*
 * Anchor commands
 */

Err _cmd_anchor_print(Session session[static 1], size_t linknum);
Err _cmd_anchor_save(Session session[static 1], size_t ix, const char* fname) ;


static inline Err _cmd_anchor_asterisk(Session session[static 1], size_t linknum) {
    try( session_follow_ahref(session, linknum));
    return Ok;
}


/*
 * Input commands
 */
Err _cmd_input_ix(Session session[static 1], const size_t ix, const char* line);


static inline Err
_get_input_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, ix);
    if (!nodeptr) return "input element number invalid";
    *outnode = *nodeptr;
    return Ok;
}

static inline Err _cmd_input_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));
    BufOf(char)* buf = &(BufOf(char)){0};
    Err err = lexbor_node_to_str(node, buf);

    ok_then(err,  session_write_msg(session, items__(buf), len__(buf)));
    buffn(char, clean)(buf);
    return err;
}

static inline Err _cmd_input_submit_ix(Session session[static 1], size_t ix) {
    return session_press_submit(session, ix);
}

/*
  Image Commands
*/

Err cmd_image(CmdParams p[static 1]);
Err _get_image_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]);
Err _cmd_image_print(Session session[static 1], size_t ix);
Err _cmd_image_save(Session session[static 1], size_t ix, const char* fname);


/*
   Misc commands
 */

Err _cmd_misc(Session session[static 1], const char* line);

static inline Err _cmd_misc_tag(const char* rest, Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return lexbor_cp_tag(rest, htmldoc->lxbdoc, textbuf_buf(htmldoc_textbuf(htmldoc)));
}

#endif
