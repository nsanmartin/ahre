#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bookmark.h"
#include "cmd-ed.h"
#include "constants.h"
#include "debug.h"
#include "mem.h"
#include "ranges.h"
#include "url.h"
#include "user-cmd.h"
#include "user-input.h"
#include "user-out.h"
#include "user-interface.h"
#include "utils.h"
#include "readpass.h"
#include "wrapper-lexbor-curl.h"

//TODO: move this to wrapper-lexbor-curl.h
Err mk_submit_url(UrlClient uc[static 1],
    lxb_dom_node_t* form,
    CURLU* out[static 1],
    HttpMethod doc_method[static 1]
);

Err read_line_from_user(Session session[static 1]) {
    char* line = 0x0;
    try( session_uinput(session)->read(session, NULL, &line));
    Err err = process_line(session, line);
    std_free(line);
    return err;
}

bool substr_match_all(const char* s, size_t len, const char* cmd) {
    return (s=csubstr_match(s, cmd, len)) && !*cstr_skip_space(s);
}

Err cmd_cookies(Session session[static 1], const char* url) {
    (void)url;
    return url_client_print_cookies(session_url_client(session));
}

Err cmd_open_url(Session session[static 1], const char* url) {
    url = cstr_trim_space((char*)url);
    if (*url == '\\') {
        Str u = (Str){0};
        try (get_url_alias(cstr_skip_space(url + 1), &u));
        Err err = session_open_url(session, u.items, session->url_client);
        str_clean(&u);
        return err;
    }

    return session_open_url(session, url, session->url_client);
}

Err doc_cmd_hide(HtmlDoc d[static 1], const char* tags) {
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) |= (~ts); }
    return err; 
}

Err doc_cmd_show(HtmlDoc d[static 1], const char* tags) {
    size_t ts = 0;
    Err err = htmldoc_tags_str_reduce_size_t(cstr_trim_space((char*)tags), &ts);
    if (!err) { *htmldoc_hide_tags(d) &= (~ts); }
    return err; 
}


static Err _get_input_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, ix);
    if (!nodeptr) return "input element number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err cmd_input_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));
    BufOf(char)* buf = &(BufOf(char)){0};
    Err err = lexbor_node_to_str(node, buf);

    ok_then(err,  session_write_msg(session, items__(buf), len__(buf)));
    buffn(char, clean)(buf);
    return err;
}


Err _cmd_input_ix(Session session[static 1], const size_t ix, const char* line) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));

    const lxb_char_t* type;
    size_t len;
    lexbor_find_attr_value(node, "type", &type, &len);

    UserOutput* out = session_uout(session);
    Err err = Ok;
    if (!*line) {
        try( out->write_std("> ", 1, session));
        ArlOf(char) masked = (ArlOf(char)){0};
        err = readpass_term(&masked, out);
        ok_then(err, lexbor_set_attr_value(node, masked.items, masked.len));
        arlfn(char, clean)(&masked);
    } else {
        if (*line == ' ' || *line == '=') ++line;
        size_t len = strlen(line);
        if (len && line[len-1] == '\n') --len;
        err = lexbor_set_attr_value(node, line, len);
    }
    ok_then(err, cmd_draw(session, ""));
    return err;
}


Err _cmd_submit_ix(Session session[static 1], size_t ix) {
    return session_press_submit(session, ix);
}

Err cmd_submit(Session session[static 1], const char* line) {
    long long unsigned num;
    try(parse_base36_or_throw(&line, &num));
    return _cmd_submit_ix(session, num);
}

Err dup_curl_with_anchors_href(lxb_dom_node_t* anchor, CURLU* u[static 1]) {

    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(anchor, "href", &data, &data_len);
    if (!data || !data_len)
        return "anchor does not have href";

    CURLU* dup = curl_url_dup(*u);
    if (!dup) return "error: memory failure (curl_url_dup)";
    BufOf(const_char)*buf = &(BufOf(const_char)){0};
    if ( !buffn(const_char, append)(buf, (const char*)data, data_len)
       ||!buffn(const_char, append)(buf, "\0", 1)
    ) {
        buffn(const_char, clean)(buf);
        curl_url_cleanup(dup);
        return "error: buffn append failure";
    }
    Err err = curlu_set_url(dup, buf->items);
    buffn(const_char, clean)(buf);
    if (err) {
        curl_url_cleanup(dup);
        return err;
    }
    *u = dup;
    return Ok;
}

Err cmd_anchor_asterisk(Session session[static 1], size_t linknum) {

    try( session_follow_ahref(session, linknum));
    return Ok;
}

Err cmd_ahre(Session session[static 1], const char* link) {
    long long unsigned linknum;
    try( parse_base36_or_throw(&link, &linknum));
    return cmd_anchor_asterisk(session, (size_t)linknum);
}

static Err _get_image_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* images = htmldoc_imgs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(images, ix);
    if (!nodeptr) return "image number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err cmd_image_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_image_by_ix(session, ix, &node));
    BufOf(char)* buf = &(BufOf(char)){0};
    Err err = lexbor_node_to_str(node, buf);
    ok_then(err, session_write_msg(session, items__(buf), len__(buf)));
    buffn(char, clean)(buf);
    return err;
}

bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    ////TODO: remove, not needed since URLU
    //return htmldoc->url && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
    return htmldoc && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

/* */

/*
 * These commands require a valid document, the caller should check this condition before
 */
Err cmd_eval(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);
    if ((rest = csubstr_match(line, "ahref", 2))) { return cmd_ahre(session, rest); } //TODO: deprecate, impl [%p
    if ((rest = csubstr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = csubstr_match(line, "class", 3))) { return "TODO: class"; }
    ///if ((rest = csubstr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = csubstr_match(line, "fetch", 1))) { return cmd_fetch(session); }
    if ((rest = csubword_match(line, "gg", 2))) { return shorcut_gg(session, rest); }
    if ((rest = csubstr_match(line, "tag", 2))) { return cmd_tag(rest, session); }
    //TODO: define shorcut_z and pass the rest
    if ((rest = csubword_match(line, "zb", 2))) { return shorcut_zb(session, rest); }
    if ((rest = csubword_match(line, "zf", 2))) { return shorcut_zf(session, rest); }
    if ((rest = csubword_match(line, "zz", 2))) { return shorcut_zz(session, rest); }

    return "unknown cmd";
}

Err cmd_anchor_print(Session session[static 1], size_t linknum) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(htmldoc);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, (size_t)linknum);
    if (!a) return "line number invalid";
    
    BufOf(char)* buf = &(BufOf(char)){0};
    try( lexbor_node_to_str(*a, buf));

    Err err = session_write_msg(session, items__(buf), len__(buf));
    buffn(char, clean)(buf);
    return err;
}


Err cmd_anchor_eval(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    //if (*line && *cstr_skip_space(line + 1)) return "error unexpected:..."; TODO: only check when necessary
    switch (*line) {
        case '\'': return cmd_anchor_print(session, (size_t)linknum); 
        case '?': return cmd_anchor_print(session, (size_t)linknum); 
        case '\0': 
        case '*': return cmd_anchor_asterisk(session, (size_t)linknum);
        default: return "?";
    }
}

Err cmd_input_eval(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    //if (*line && *cstr_skip_space(line + 1)) return "error unexpected:..."; TODO ^
    switch (*line) {
        case '?': return cmd_input_print(session, linknum);
        case '\0': 
        case '*': return _cmd_submit_ix(session, linknum);
        case '=': return _cmd_input_ix(session, linknum, line + 1); 
        default: return "?";
    }
}

Err cmd_image_eval(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return cmd_image_print(session, linknum);
        default: return "?";
    }
    return Ok;
}

Err tabs_eval(Session session[static 1], const char* line) {
    TabList* f = session_tablist(session);

    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return tablist_print_info(session, f);
        case '-': return tablist_back(f);
        default: return tablist_move_to_node(f, line);
    }
    return Ok;
}

Err doc_eval_word(HtmlDoc d[static 1], const char* line) {
    const char* rest;
    if ((rest = csubstr_match(line, "hide", 1))) { return doc_cmd_hide(d, rest); }
    if ((rest = csubstr_match(line, "show", 1))) { return doc_cmd_show(d, rest); }
    return "unknown doc command";
}

Err dbg_traversal(Session ctx[static 1], HtmlDoc d[static 1], const char* f) ;
Err doc_eval(HtmlDoc d[static 1], const char* line, Session session[static 1]) {
    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return htmldoc_print_info(session, d);
        case 'A': return htmldoc_A(session, d);
        case '+': return bookmark_add_doc(d, cstr_skip_space(line + 1), session_url_client(session));
        case '%': return dbg_traversal(session, d, line + 1);
        default: return doc_eval_word(d, line);
    }
}

//TODO: new user interface:
// first non space char identify th resource:
// { => anchor
// [ => image
// < => inputn
// etc
//
// Then a range is read (range not of lines but elements)
// ' => show
// * => go
// = => set
//
// when they make sense: a text input can be set, an anchor no, etc.
Err process_line(Session session[static 1], const char* line) {
    if (!line) { session_quit_set(session); return "no input received, exiting"; }
    line = cstr_skip_space(line);
    if (*line == '\\') line = cstr_skip_space(line + 1);
    if (!*line) { return Ok; }
    const char* rest = NULL;
    /* these commands does not require current valid document */
    if ((rest = csubstr_match(line, "bookmarks", 1))) { return cmd_bookmarks(session, rest); }
    if ((rest = csubstr_match(line, "cookies", 1))) { return cmd_cookies(session, rest); }
    //TODO@ if ((rest = csubstr_match(line, "dbg", 1))) return cmd_dbg(session, rest)
    if ((rest = csubstr_match(line, "echo", 1))) return puts(rest) < 0 ? "error: puts failed" : Ok;
    if ((rest = csubstr_match(line, "go", 1))) { return cmd_open_url(session, rest); }
    if ((rest = csubstr_match(line, "quit", 1)) && !*rest) { session_quit_set(session); return Ok;}
    if ((rest = csubstr_match(line, "set", 1))) { return cmd_set(session, rest); }

    if (*line == '|') return tabs_eval(session, line + 1);
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    if (!htmldoc_is_valid(htmldoc) ||!session->url_client) return "no document";

    if (*line == '.') return doc_eval(htmldoc, line + 1, session);
    //TODO: implement search in textbuf
    if (*line == '/') return "to search in buffer use ':/' (not just '/')";

    //TODO: obtain range from line and pase it already parsed to eval fn

    if ((rest = csubstr_match(line, "draw", 1))) { return cmd_draw(session, rest); }

    TextBuf* tb;
    try( session_current_buf(session, &tb));
    if (*line == ':') return ed_eval(session, tb, line + 1);
    if (*line == '<') return ed_eval(session, htmldoc_sourcebuf(htmldoc), line + 1);
    if (*line == '&') return dbg_print_form(session, line + 1);//TODO: form is &Form or something else
    if (*line == ANCHOR_OPEN_STR[0]) return cmd_anchor_eval(session, line + 1);
    if (*line == INPUT_OPEN_STR[0]) return cmd_input_eval(session, line + 1);
    if (*line == IMAGE_OPEN_STR[0]) return cmd_image_eval(session, line + 1);

    return cmd_eval(session, line);
}

Err process_line_line_mode(Session* s, const char* line) {
    if (!s) return "error: no session :./";
    return process_line(s, line);
}

Err process_line_vi_mode(Session* s, const char* line) {
    if (!s) return "error: no session :./";
    try( process_line(s, line));
    return cmd_draw(s, "");//TODO: do not rewrite the title
}

#include <sys/ioctl.h>

Err ui_get_win_size(size_t nrows[static 1], size_t ncols[static 1]) {
    struct winsize w;
    if (-1 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
        return err_fmt("error: ioctl failure: %s", strerror(errno));
    if (nrows) *nrows = w.ws_row;
    if (ncols) *ncols = w.ws_col;
    return Ok;
}
