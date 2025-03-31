#ifndef __AHRE_CMD_H__
#define __AHRE_CMD_H__

#include "error.h"
#include "session.h"
#include "ahre__.h"

/*
 * Session commands
 */

Err cmd_open_url(Session session[static 1], const char* url);

static inline Err cmd_cookies(Session session[static 1], const char* url) {
    (void)url;
    return url_client_print_cookies(session_url_client(session));
}

static inline Err cmd_draw(Session session[static 1], const char* rest) {
    if (*rest) return "draw cmd accept no params";
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    htmldoc_reset(htmldoc);
    return htmldoc_draw(htmldoc, session);
}

/*
 * Tabs commands
 */

Err cmd_tabs(Session session[static 1], const char* line);

/* 
 * HtmlDoc commands
 */
Err dbg_traversal(Session ctx[static 1], HtmlDoc d[static 1], const char* f) ;
Err cmd_doc(HtmlDoc d[static 1], const char* line, Session session[static 1]);


static inline Err cmd_doc_hide(HtmlDoc d[static 1], const char* tags) {
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) |= (~ts); }
    return err; 
}

static inline Err cmd_doc_show(HtmlDoc d[static 1], const char* tags) {
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) &= (~ts); }
    return err; 
}

static inline Err doc_eval_word(HtmlDoc d[static 1], const char* line) {
    const char* rest;
    if ((rest = csubstr_match(line, "hide", 1))) { return cmd_doc_hide(d, rest); }
    if ((rest = csubstr_match(line, "show", 1))) { return cmd_doc_show(d, rest); }
    return "unknown doc command";
}

/*
 * TextBuf commands
 */

Err dbg_print_all_lines_nums(Session s[static 1], TextBuf textbuf[static 1]);
//TODO: pass session to this fn
static inline Err cmd_textbuf_print_last_range(Session s[static 1], TextBuf textbuf[static 1]) {
    Range r = *textbuf_last_range(textbuf);
    try( session_write_msg_lit__(s, "last range: ("));
    try( session_write_unsigned_msg(s, r.beg));
    try( session_write_msg_lit__(s, ", "));
    try( session_write_unsigned_msg(s, r.end));
    try( session_write_msg_lit__(s, ")\n"));
    return Ok;
}

static inline Err cmd_textbuf_print_all(TextBuf textbuf[static 1]) {
    BufOf(char)* buf = &textbuf->buf;
    fwrite(buf->items, 1, buf->len, stdout);
    return NULL;
}

static inline Err
cmd_textbuf_print(Session s[static 1], TextBuf textbuf[static 1], Range range[static 1]) {
    SessionMemWriter writer = (SessionMemWriter){.s=s, .write=session_writer_write_std};
    return session_write_range_mod(&writer, textbuf, range);
}
Err cmd_textbuf(Session s[static 1], TextBuf textbuf[static 1], const char* line);

Err cmd_textbuf_eval(
    Session s[static 1], TextBuf textbuf[static 1], const char* line, Range range[static 1]);

/*
 * Anchor commands
 */

Err cmd_anchor(Session session[static 1], const char* line);

static inline Err cmd_anchor_asterisk(Session session[static 1], size_t linknum) {
    try( session_follow_ahref(session, linknum));
    return Ok;
}


/*
 * Input commands
 */
Err cmd_input(Session session[static 1], const char* line);

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


static inline Err cmd_input_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));
    BufOf(char)* buf = &(BufOf(char)){0};
    Err err = lexbor_node_to_str(node, buf);

    ok_then(err,  session_write_msg(session, items__(buf), len__(buf)));
    buffn(char, clean)(buf);
    return err;
}

static inline Err cmd_input_submit_ix(Session session[static 1], size_t ix) {
    return session_press_submit(session, ix);
}

///static inline Err cmd_submit(Session session[static 1], const char* line) {
///    long long unsigned num;
///    try(parse_base36_or_throw(&line, &num));
///    return cmd_input_submit_ix(session, num);
///}

/*
  Image Commands
*/

Err cmd_image_eval(Session session[static 1], const char* line);
Err _get_image_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]);
Err cmd_image_print(Session session[static 1], size_t ix);


/*
   Misc commands
 */

Err cmd_misc(Session session[static 1], const char* line);

static inline Err cmd_misc_tag(const char* rest, Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    return lexbor_cp_tag(rest, htmldoc->lxbdoc, textbuf_buf(htmldoc_textbuf(htmldoc)));
}

#endif
