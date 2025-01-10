#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "src/cmd-ed.h"
#include "src/constants.h"
#include "src/debug.h"
#include "src/mem.h"
#include "src/ranges.h"
#include "src/url.h"
#include "src/user-cmd.h"
#include "src/user-input.h"
#include "src/user-interface.h"
#include "src/utils.h"
#include "src/wrapper-lexbor-curl.h"

//TODO: move this to wrapper-lexbor-curl.h
Err mk_submit_url(UrlClient uc[static 1],
    lxb_dom_node_t* form,
    CURLU* out[static 1],
    HttpMethod doc_method[static 1]
);

Err read_line_from_user(Session session[static 1]) {
    char* line = 0x0;
    line = read_user_input(NULL);
    Err err = session->user_line_callback(session, line);
    destroy(line);
    return err;
}

bool substr_match_all(const char* s, size_t len, const char* cmd) {
    return (s=substr_match(s, cmd, len)) && !*cstr_skip_space(s);
}


Err cmd_cookies(Session session[static 1], const char* url) {
    (void)url;
    return url_client_print_cookies(session_url_client(session));
}

///TODO: move this to session
Err cmd_set_url(Session session[static 1], const char* url) {
    HtmlDoc* htmldoc = session_current_doc(session);

    url = cstr_trim_space((char*)url);
    if (!*url) {
        char* buf;
        try(url_cstr(htmldoc_url(htmldoc), &buf));
        Err res = err_fmt("current url: '%s'", buf);
        curl_free(buf);
        return res;
    }

    HtmlDoc* newdoc = htmldoc_create(url);
    if (!newdoc) { return err_fmt("error: could not create doc from %s", url); }

    Err err = htmldoc_fetch(newdoc, session->url_client);
    if (err) {
        htmldoc_destroy(newdoc);
        return err;
    }

    htmldoc_destroy(htmldoc);
    session->htmldoc = newdoc;
    //TODO: print curlu
    //printf("set with url: %s\n", newdoc->url ? newdoc->url : "<no url>");
    return Ok;
}


static Err _get_input_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, ix);
    if (!nodeptr) return "input element number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err cmd_input_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));
    BufOf(const_char)* buf = &(BufOf(const_char)){0};
    //TODO: cleanup on failure
    try( lexbor_node_to_str(node, buf));
    bool write_err = fwrite(buf->items, sizeof(char), buf->len, stdout) == buf->len;
    buffn(const_char, clean)(buf);
    return write_err ? Ok : "error: fwrite failure";
}

Err cmd_input(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, linknum, &node));
    if (!*line) return "borrar?";
    if (*line == ' ' || *line == '=') ++line;

    try(lexbor_set_attr_value(node, "value", line));

    return Ok;
}

Err _cmd_input_ix(Session session[static 1], const size_t ix, const char* line) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));
    if (!*line) return "borrar?";
    if (*line == ' ' || *line == '=') ++line;

    try(lexbor_set_attr_value(node, "value", line));

    return Ok;
}


Err _cmd_submit_ix(Session session[static 1], size_t ix) {
    Err err = Ok;
    HtmlDoc* newdoc;

    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(htmldoc);
    CURLU* curlu = curl_url_dup(url_cu(htmldoc_url(htmldoc)));
    if (!curlu) return "error: memory failure (curl_url_dup)";



    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, ix);
    if (!nodeptr) {
        err="link number invalid";
        goto cleanup_curlu;
    }
    if (!_lexbor_attr_has_value(*nodeptr, "type", "submit")) {
        err="warn: not submit input";
        goto cleanup_curlu;
    }

    lxb_dom_node_t* form = _find_parent_form(*nodeptr);
    if (form) {

   
        UrlClient* url_client = session_url_client(session);
        HttpMethod method;
        if ((err = mk_submit_url(url_client, form, &curlu, &method))) {
            curl_url_cleanup(curlu);
            return err;
        }
        ////TODO: all this is duplicated, refactor!
        if (!(newdoc=htmldoc_create(NULL))) { err="error creating htmldoc"; goto cleanup_curlu; };
        htmldoc_url(newdoc)->cu=curlu;
        newdoc->method = method;

        if ((err=htmldoc_fetch(newdoc, session_url_client(session)))) {
           goto cleanup_htmldoc; 
        }

        if ((err=htmldoc_browse(newdoc))) {
           goto cleanup_htmldoc; 
        }

        session->htmldoc = newdoc;
        htmldoc_destroy(htmldoc);
        //TODO: change this when multi docs gets implemented
    
    } else { puts("expected form, not found"); }
    return Ok;
cleanup_htmldoc:
    htmldoc_destroy(newdoc);
cleanup_curlu:
    curl_url_cleanup(curlu);
    return err;
}

Err cmd_submit(Session session[static 1], const char* line) {
    long long unsigned num;
    try(parse_base36_or_throw(&line, &num));
    return _cmd_submit_ix(session, num);
}

Err dup_curl_with_anchors_href(
    lxb_dom_node_t* anchor,
    CURLU* u[static 1]
) {

    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(anchor, "href", &data, &data_len);
    if (!data || !data_len)
        return "anchor does not have href";

    CURLU* dup = curl_url_dup(*u);
    if (!dup) return "error: memory failure (curl_url_dup)";
    BufOf(const_char)*buf = &(BufOf(const_char)){0};
    if ( !buffn(const_char, append)(buf, (const char*)data, data_len)
       ||!buffn(const_char, append)(buf, "\0", 1)) {
        curl_url_cleanup(dup);
        return "error: buffn append failure";
    }
    Err err = curlu_set_url(dup, buf->items);
    buffn(const_char, clean)(buf);
    if (err) { /// /*todo cleanup */ return err;
        curl_url_cleanup(dup);
        return err;
    }
    *u = dup;
    return Ok;
}

Err cmd_anchor_asterisk(Session session[static 1], size_t linknum) {
    Err err;
    HtmlDoc* newdoc;

    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(htmldoc);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, linknum);
    if (!a) return "link number invalid";

    CURLU* curlu = url_cu(htmldoc_url(htmldoc));
    try( dup_curl_with_anchors_href(*a, &curlu));

    //TODO: remove duplicte code.
    if (!(newdoc=htmldoc_create(NULL))) {
        curl_url_cleanup(curlu);
        return "error creating htmldoc";
    };

    htmldoc_url(newdoc)->cu=curlu;
    if ((err=htmldoc_fetch(newdoc, session_url_client(session)))) {
        htmldoc_destroy(newdoc);
        return err;
    }

    if ((err=htmldoc_browse(newdoc))) {
        htmldoc_destroy(newdoc);
        return err;
    }

    session->htmldoc = newdoc;
    htmldoc_destroy(htmldoc);
    return Ok;
}

Err cmd_ahre(Session session[static 1], const char* link) {
    long long unsigned linknum;
    try( parse_base36_or_throw(&link, &linknum));
    return cmd_anchor_asterisk(session, (size_t)linknum);
}

static Err _get_image_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(LxbNodePtr)* images = htmldoc_imgs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(images, ix);
    if (!nodeptr) return "image number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err cmd_image_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_image_by_ix(session, ix, &node));
    BufOf(const_char)* buf = &(BufOf(const_char)){0};
    //TODO: cleanup on failure
    try( lexbor_node_to_str(node, buf));
    bool write_err = fwrite(buf->items, sizeof(char), buf->len, stdout) == buf->len;
    buffn(const_char, clean)(buf);
    return write_err ? Ok : "error: fwrite failure";
}

bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    ////TODO: remove, not needed since URLU
    //return htmldoc->url && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
    return htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

/* */

/*
 * These commands require a valid document, the caller should check this condition before
 */
Err cmd_eval(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);
    if ((rest = substr_match(line, "ahref", 2))) { return cmd_ahre(session, rest); }
    if ((rest = substr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = substr_match(line, "class", 3))) { return "TODO: class"; }
    if ((rest = substr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = substr_match(line, "fetch", 1))) { return cmd_fetch(session); }
    if ((rest = substr_match(line, "input", 1))) { return cmd_input(session, rest); }
    if ((rest = substr_match(line, "submit", 1))) { return cmd_submit(session, rest); }
    if ((rest = substr_match(line, "tag", 2))) { return cmd_tag(rest, session); }
    if ((rest = substr_match(line, "text", 2))) { return cmd_text(session); }

    return "unknown cmd";
}

//TODO: sotr lxb_node_t* and implement print verbose like the input one
//Err cmd_anchor_print_verbose(Session session[static 1], size_t ix) {
//    lxb_dom_node_t* node;
//    try( _get_input_by_ix(session, ix, &node));
//    BufOf(const_char)* buf = &(BufOf(const_char)){0};
//    //TODO: cleanup on failure
//    try( lexbor_node_to_str(node, buf));
//    bool write_err = fwrite(buf->items, sizeof(char), buf->len, stdout) == buf->len;
//    buffn(const_char, clean)(buf);
//    return write_err ? Ok : "error: fwrite failure";
//}

Err cmd_anchor_print(Session session[static 1], size_t linknum) {
    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(htmldoc);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, (size_t)linknum);
    if (!a) return "line number invalid";
    
    BufOf(const_char)* buf = &(BufOf(const_char)){0};
    try( lexbor_node_to_str(*a, buf));
    bool write_err = fwrite(buf->items, sizeof(char), buf->len, stdout) == buf->len;
    buffn(const_char, clean)(buf);
    return write_err ? Ok : "error: fwrite failure";
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
    if (!line) { session->quit = true; return "no input received, exiting"; }
    line = cstr_skip_space(line);
    if (!*line) { return Ok; }
    const char* rest = NULL;
    /* these commands does not require current valid document */
    if ((rest = substr_match(line, "browse", 1))) { return cmd_browse(session, rest); }
    if ((rest = substr_match(line, "cookies", 1))) { return cmd_cookies(session, rest); }
    if ((rest = substr_match(line, "echo", 1))) return puts(rest) < 0 ? "error: puts failed" : Ok;
    if ((rest = substr_match(line, "go", 1))) { return cmd_browse(session, rest); }
    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return Ok;}
    if ((rest = substr_match(line, "url", 1))) { return cmd_set_url(session, rest); }

    if (!htmldoc_is_valid(session_current_doc(session)) ||!session->url_client) return "no document";

    //TODO: implement search in textbuf
    if (*line == '/') return "/ (search) not implemented";

    //TODO: obtain range from line and pase it already parsed to eval fn

    if (*line == ':') return ed_eval(session, line + 1);
    if (*line == ANCHOR_OPEN_STR[0]) return cmd_anchor_eval(session, line + 1);
    if (*line == INPUT_OPEN_STR[0]) return cmd_input_eval(session, line + 1);
    if (*line == IMAGE_OPEN_STR[0]) return cmd_image_eval(session, line + 1);

    return cmd_eval(session, line);
}
