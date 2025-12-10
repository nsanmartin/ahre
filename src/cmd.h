#ifndef __AHRE_CMD_H__
#define __AHRE_CMD_H__

#include "error.h"
#include "session.h"
#include "draw.h"

#define cmd_assert_no_params(Ln) do{ if(*Ln) return "error: expecting no params"; }while(0)

// typedef enum { cmd_base_tag, cmd_textbuf_tag } CmdTag;
typedef struct {
    const char* ln;
    Session*               s;
    TextBuf*               tb; /* in order to reuse buf cmds for source & buf, we pass it */
    RangeParse       rp;
    size_t                 ix; /* TODO: deprecate this and use only range */
} CmdParams;

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
/*
 * Curl commands
 */

#define CMD_CURL_COOKIES_DOC \
    "Show the cookies.\n"
static inline Err cmd_curl_cookies(CmdParams p[_1_]) {
    return url_client_print_cookies(p->s, session_url_client(p->s));
}

#define CMD_CURL_VERSION_DOC \
    "Shows curl's version.\n"
static inline Err cmd_curl_version(CmdParams p[_1_]) {
    if (!p->s) return "error: session is null";
    char* version = curl_version();
    return session_write_msg_ln(p->s, version, strlen(version));
}

// curl commands

Err htmldoc_redraw(HtmlDoc htmldoc[_1_], Session s[_1_]);
static inline Err session_doc_draw(Session session[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return htmldoc_redraw(htmldoc, session);
}

static inline Err session_doc_js(Session session[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return htmldoc_switch_js(htmldoc, session);
}

static inline Err session_doc_console(Session session[_1_], const char* line) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return htmldoc_console(htmldoc, session, line);
}


#define CMD_DOC_DRAW \
    "Redraws the current document.\n"
static inline Err cmd_doc_draw(CmdParams p[_1_]) { return session_doc_draw(p->s); }

#define CMD_DOC_JS \
    "Switch js engine for doc.\n"
static inline Err cmd_doc_js(CmdParams p[_1_]) { return session_doc_js(p->s); }

#define CMD_DOC_CONSOLE \
    "js engine console\n"
static inline Err cmd_doc_console(CmdParams p[_1_]) {
    return session_doc_console(p->s, p->ln);
}

/*
 * Tabs commands
 */

Err cmd_tabs(CmdParams p[_1_]);

static inline Err cmd_tabs_info(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    TabList* f = session_tablist(p->s);
    return tablist_info(p->s, f);
}

static inline Err cmd_tabs_back(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    TabList* f = session_tablist(p->s);
    return tablist_back(f);
}

static inline Err cmd_tabs_goto(CmdParams p[_1_]) {
    TabList* f = session_tablist(p->s);
    return tablist_move_to_node(f, p->ln);
}

/* 
 * HtmlDoc commands
 */
Err _cmd_doc_dbg_traversal(Session ctx[_1_], const char* f);
Err cmd_doc(CmdParams p[_1_]);

#define CMD_DOC_INFO_DOC \
    "Prints document information such as the title and its url.\n"
static inline Err cmd_doc_info(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return htmldoc_print_info(p->s, d);
}

static inline Err cmd_doc_A(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return htmldoc_A(p->s, d);
}

Err bookmark_add_to_section(Session s[_1_], const char* line, UrlClient url_client[_1_]);
#define CMD_DOC_BOOKMARK_ADD \
    ".+[/]SECTION_NAME\n\n"\
    "Adds current document to bookmarks.\n\n"\
    "If section name is preceeded by '/', te substring given is used to\n"\
    "match a section name in the bookmark.\n"\
    "If not, the section is created unless there is a complete match in which \n"\
    "case the document is added to it.\n"
static inline Err cmd_doc_bookmark_add(CmdParams p[_1_]) {
    return bookmark_add_to_section(p->s, p->ln, session_url_client(p->s));
}

static inline Err _cmd_doc_hide(Session session[_1_], const char* tags) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) |= (~ts); }
    return err; 
}

static inline Err _cmd_doc_show(Session session[_1_], const char* tags) {
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

Err cmd_textbuf_global(CmdParams p[_1_]);
StrView parse_pattern(const char* tk);


typedef Err (*TextBufCmdFn)
    (Session s[_1_], TextBuf tb[_1_], Range r[_1_], const char* ln);


static inline Err cmd_textbuf_print(CmdParams p[_1_]) {
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return session_write_std_range_mod(p->s, p->tb, &rng);
}


Err dbg_print_all_lines_nums(
    Session s[_1_], TextBuf tb[_1_], Range r[_1_], const char* ln
);

static inline Err
cmd_textbuf_dbg_print_all_lines_nums(CmdParams p[_1_]) {
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return dbg_print_all_lines_nums(p->s, p->tb, &rng, p->ln);
}


Err _textbuf_print_n_(
    Session s[_1_], TextBuf textbuf[_1_], Range range[_1_], const char* ln);

static inline Err
cmd_textbuf_print_n(CmdParams p[_1_]) {
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return  _textbuf_print_n_(p->s, p->tb, &rng, p->ln);
}

Err _cmd_textbuf_write_impl(
   Session s[_1_], TextBuf textbuf[_1_], Range r[_1_], const char* rest
);

static inline Err cmd_textbuf_write(CmdParams p[_1_]) {
//TODO: allow >/>> modes
    const char* filename = cstr_trim_space((char*)p->ln);
    if (range_parse_is_none(&p->rp)) {
        try( textbuf_to_file(p->tb, filename, "w"));
        return session_write_msg_lit__(p->s, "file written.");
    }
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return _cmd_textbuf_write_impl(p->s, p->tb, &rng, filename);
}

/*
 * Anchor commands
 */

Err _cmd_anchor_print(Session session[_1_], size_t linknum);
Err _cmd_anchor_save(Session session[_1_], size_t ix, const char* fname) ;


static inline Err _cmd_anchor_asterisk(Session session[_1_], size_t linknum) {
    try( session_follow_ahref(session, linknum));
    return Ok;
}


/*
 * Input commands
 */
Err _cmd_input_ix_set_(Session session[_1_], const size_t ix, const char* line);


static inline Err
_get_lexbor_node_ptr_by_ix(ArlOf(LxbNodePtr) node_arl[_1_], size_t ix, lxb_dom_node_t* outnode[_1_]) {
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(node_arl, ix);
    if (!nodeptr) return "input element number invalid";
    *outnode = *nodeptr;
    return Ok;
}


static inline Err _cmd_lexbor_node_print_(
    Session           session[_1_],
    ArlOf(LxbNodePtr) node_arl[_1_],
    size_t            ix
) {
    lxb_dom_node_t* node;
    try( _get_lexbor_node_ptr_by_ix(node_arl, ix, &node));
    Str* buf = &(Str){0};
    Err err = lexbor_node_to_str(node, buf);

    ok_then(err,  session_write_msg(session, items__(buf), len__(buf)));
    str_clean(buf);
    return err;
}

static inline Err _cmd_form_print(Session s[_1_], size_t ix) {
    HtmlDoc* d;
    try( session_current_doc(s, &d));
    return _cmd_lexbor_node_print_(s, htmldoc_forms(d), ix);
}

static inline Err _cmd_input_print(Session s[_1_], size_t ix) {
    HtmlDoc* d;
    try( session_current_doc(s, &d));
    return _cmd_lexbor_node_print_(s, htmldoc_inputs(d), ix);
}

static inline Err _cmd_input_submit_ix(Session session[_1_], size_t ix) {
    return session_press_submit(session, ix);
}

static inline Err cmd_select_elem_show_options(LxbNode lbn[_1_], Session s[_1_]) {
    if (!lxbnode_node(lbn)) return "error: expecting lexbor node, not NULL";
    for(lxb_dom_node_t* it = lxbnode_node(lbn)->first_child; it ; it = it->next) {
        if (it->local_name == LXB_TAG_OPTION) {

            if ( lexbor_has_lit_attr__(it, "selected")) session_write_msg_lit__(s, " *\t");
            else session_write_msg_lit__(s, "\t");

            lxb_dom_node_t* opt_text = it->first_child;
            if (opt_text->local_name == LXB_TAG__TEXT) {
                const char* text;
                size_t len;
                try(lexbor_get_text(opt_text, &text, &len));
                if (len) try( session_write_msg(s, (char*)text, len));
            }

            session_write_msg_lit__(s, " | value: "); 
            StrView value = lexbor_get_lit_attr__(it, "value");
            if (value.len) session_write_msg_ln(s, (char*)value.items, value.len);
            else session_write_msg_lit__(s, "\"\"\n"); 

        }
    }
    return Ok;
}


static inline Err cmd_input_default_ix(Session s[_1_], size_t ix) {
    TabNode* tab;
    HtmlDoc* doc;
    LxbNode  lbn;
    try( tablist_current_tab(session_tablist(s), &tab));
    try( tab_node_current_doc(tab, &doc));
    try( htmldoc_input_at(doc, ix, &lbn));

    if (lexbor_node_tag_is_input(&lbn)
    &&  lexbor_lit_attr_has_lit_value(&lbn, "type", "submit")) 
        return tab_node_tree_append_submit(tab , ix, session_url_client(s), s);
    else if (lexbor_node_tag_is_button(&lbn)
    &&  (lexbor_lit_attr_has_lit_value(&lbn, "type", "submit") 
        || !lexbor_has_lit_attr__(&lbn, "type")))
        return tab_node_tree_append_submit(tab , ix, session_url_client(s), s);
    else if (lexbor_node_tag_is_select(&lbn))
        return cmd_select_elem_show_options(&lbn, s);
    
    return "error: invalid input node";
}

/*
  Image Commands
*/

Err cmd_image(CmdParams p[_1_]);
Err _get_image_by_ix(Session session[_1_], size_t ix, lxb_dom_node_t* outnode[_1_]);
Err _cmd_image_print(Session session[_1_], size_t ix);
Err _cmd_image_save(Session session[_1_], size_t ix, const char* fname);


/*
   Misc commands
 */

Err _cmd_misc(Session session[_1_], const char* line);

static inline Err _cmd_misc_tag(const char* rest, Session session[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return lexbor_cp_tag(rest, htmldoc->lxbdoc, textbuf_buf(htmldoc_textbuf(htmldoc)));
}

Err cmd_fetch(Session session[_1_]);
Err cmd_set_session_forms(CmdParams p[_1_]);
#endif
