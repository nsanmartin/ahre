#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "src/debug.h"
#include "src/mem.h"
#include "src/ranges.h"
#include "src/user-cmd.h"
#include "src/user-interface.h"
#include "src/utils.h"
#include "src/cmd-ed.h"

Err read_line_from_user(Session session[static 1]) {
    char* line = 0x0;
    line = readline("");
    Err err = session->user_line_callback(session, line);
    if (line && cstr_skip_space(line)) {
        add_history(line);
    }
    destroy(line);
    return err;
}

bool substr_match_all(const char* s, size_t len, const char* cmd) {
    return (s=substr_match(s, cmd, len)) && !*cstr_skip_space(s);
}


///TODO: move this to session
Err cmd_set_url(Session session[static 1], const char* url) {
    HtmlDoc* htmldoc = session_current_doc(session);
    url = cstr_trim_space((char*)url);
    if (!*url) return "empty url, not setting";
    if (htmldoc->url && strcmp(url, htmldoc->url) == 0) 
        return "same url, not updating";

    htmldoc_cleanup(htmldoc);
    if (htmldoc_init(htmldoc, url)) {
        return "error: could not init htmldoc";
    }

    if (htmldoc_has_url(htmldoc)) {
        Err err = htmldoc_fetch(htmldoc, session->url_client);
        if (err) 
            return err;
        
    }
    printf("set with url: %s\n", htmldoc->url ? htmldoc->url : "<no url>");
    return Ok;
}

Err parse_number_or_throw(const char** strptr, unsigned long long* num) {
    if (!strptr || !*strptr) return "error: unexpected NULL ptr";
    while(**strptr && isspace(**strptr)) ++*strptr;
    if (!**strptr) return "number not given";
    if (!isdigit(**strptr)) return "number must consist in digits";
    char* endptr = NULL;
    *num = strtoull(*strptr, &endptr, 10);
    if (errno == ERANGE)
        return "warn: number out of range";
    if (*strptr == endptr)
        return "warn: number could not be parsed";
    if (*num > SIZE_MAX) 
        return "warn: number out of range (exceeds size max)";
    return Ok;
}


static lxb_dom_node_t*
_find_parent_form(lxb_dom_node_t* node) {
    for (;node; node = node->parent) {
        if (node->local_name == LXB_TAG_FORM) break;
    }
    return node;
}


static Err _make_submit_url_rec(
    UrlClient url_client[static 1], lxb_dom_node_t node[static 1], BufOf(char) submit_url[static 1]
) {
    char* escaped;
    if (!node) return Ok;
    else if (node->local_name == LXB_TAG_INPUT) {

        const lxb_char_t* value;
        size_t valuelen;
        lexbor_find_attr_value(node, "value", &value, &valuelen);
        if (valuelen > 0) {
            escaped = url_client_escape_url(url_client, (char*)value, valuelen);
            if (!escaped) return "error: curl_escape failure";
            const lxb_char_t* name;
            size_t namelen;
            lexbor_find_attr_value(node, "name", &name, &namelen);
            if (!name || !value || namelen == 0) goto free_escaped;
            //if (!name || !value || namelen == 0) return "error: lexbor tree failure";
            if (!buffn(char, append)(submit_url, "&", 1)) goto free_escaped;
            //if (!buffn(char, append)(submit_url, "&", 1)) return "error: buffn failure";
            if (!buffn(char, append)(submit_url, (char*)name, namelen)) goto free_escaped;
            //if (!buffn(char, append)(submit_url, (char*)name, namelen)) return "error: buffn failure";
            if (!buffn(char, append)(submit_url, "=", 1)) goto free_escaped;
            //if (!buffn(char, append)(submit_url, "=", 1)) return "error: buffn failure";
            if (!buffn(char, append)(submit_url, escaped, strlen(escaped))) goto free_escaped;
            //if (!buffn(char, append)(submit_url, escaped, strlen(escaped))) {
            //    //TODO: this in all tries
            //    url_client_curl_free_cstr(escaped);
            //    return "error: buffn failure";
            //}
            url_client_curl_free_cstr(escaped);
        }
    } 

    for(lxb_dom_node_t* it = node->first_child; ; it = it->next) {
        Err err = _make_submit_url_rec(url_client, it, submit_url);
        if (err) return err;
        if (it == node->last_child) { break; }
    }
    return Ok;
free_escaped:
    url_client_curl_free_cstr(escaped);
    return err_fmt("error: failure in %s", __func__);
}


static Err make_submit_url(
    UrlClient url_client[static 1], lxb_dom_node_t form[static 1], BufOf(char) submit_url[static 1]
) {
    Err err = Ok;

    try(err, _make_submit_url_rec(url_client, form, submit_url));
    if (!buffn(char, append)(submit_url, "\0", 1)) return "error: buffn failure";
    return Ok;
}


Err cmd_input(Session session[static 1], const char* line) {
    Err err = Ok;
    long long unsigned num;
    BufOf(char)* submit_url = &(BufOf(char)){0};
    HtmlDoc* newdoc;

    try(err, parse_number_or_throw(&line, &num));
    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(htmldoc);


    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, (size_t)num);
    if (!nodeptr) return "link number invalid";

    lxb_dom_node_t* form = _find_parent_form(*nodeptr);
    if (form) {

        if (!buffn(char, append)(submit_url, (char*)htmldoc->url, strlen(htmldoc->url))) return "error: buffn failure";
        //defer
        const lxb_char_t* action;
        size_t action_len;
        lexbor_find_attr_value(form, "action", &action, &action_len);
        if(!buffn(char, append)(submit_url, (char*)action, action_len)) goto free_sumbit_url;
        if(!buffn(char, append)(submit_url, "?", 1)) goto free_sumbit_url;

        trygotoerr (err, free_sumbit_url, make_submit_url(session->url_client, form, submit_url));
        //try (err, make_submit_url(session->url_client, form, submit_url));
        //if (!submit_url) return "error: no url made"; not possible

        puts("The submit URL:"); puts(submit_url->items);
        //TODO: all this is duplicated, refactor!
        newdoc = htmldoc_create(submit_url->items);
        if (!newdoc) { err="error creating htmldoc"; goto free_sumbit_url; };
        //if (!newdoc) { buffn(char, clean)(submit_url); return "error: could not create html doc"; }
        buffn(char, clean)(submit_url);
        if (newdoc->url && newdoc->lxbdoc) {
            //try(err, htmldoc_fetch(newdoc, session_url_client(session)));
            trygotoerr(err, destroy_new_htmldoc, htmldoc_fetch(newdoc, session_url_client(session)));
            //if ((err = htmldoc_fetch(newdoc, session_url_client(session)))) { buffn(char, clean)(submit_url); return err; }

            //try(err, htmldoc_browse(newdoc));
            trygotoerr(err, destroy_new_htmldoc, htmldoc_browse(newdoc));
            //if ((err = htmldoc_browse(newdoc))) { buffn(char, clean)(submit_url); return err; }
        }
        htmldoc_destroy(htmldoc);
        //TODO: change this when multi docs gets implemented
        session->htmldoc = newdoc;
    
    } else { puts("expected form, not found"); }
    buffn(char, clean)(submit_url);
    return Ok;
destroy_new_htmldoc:
        htmldoc_destroy(newdoc);
        return "error fetching or browsing doc";
free_sumbit_url:
        buffn(char, clean)(submit_url);
        return err ? err : err_fmt("error in: %", __func__);
}

//TODO: remove duplicte code.
Err cmd_ahre(Session session[static 1], const char* link) {
    if (!link) return "error: unexpected nullptr";
    while(*link && isspace(*link)) ++link;
    if (!*link) return "link number not given";
    if (!isdigit(*link)) return "link number mut consist in digits";
    char* endptr = NULL;
    long long unsigned linknum = strtoull(link, &endptr, 10);
    if (errno == ERANGE)
        return "link number out of range";
    if (link == endptr)
        return "link number could not be parsed";
    if (linknum > SIZE_MAX) 
        return "link number out of range (exceeds size max)";

    HtmlDoc* htmldoc = session_current_doc(session);
    ArlOf(Ahref)* hrefs = htmldoc_ahrefs(htmldoc);

    Ahref* ahref = arlfn(Ahref, at)(hrefs, (size_t)linknum);
    if (!ahref) return "link number invalid";

    Err e = Ok;
    const char* url = htmldoc_abs_url_dup(htmldoc, ahref->url);
    if (!url) return "error: memory failure";
    HtmlDoc* newdoc = htmldoc_create(cstr_trim_space((char*)url));
    if (!newdoc) {
        std_free((char*)url);
        return "error: could not create html doc";
    }
    if (newdoc->url && newdoc->lxbdoc) {
        if ((e = htmldoc_fetch(newdoc, session_url_client(session)))) {
            std_free((char*)url);
            return e;
        }

        if ((e = htmldoc_browse(newdoc))) {
            std_free((char*)url);
            return e;
        }
    }
    htmldoc_destroy(htmldoc);
    //TODO: change this when multi docs gets implemented
    session->htmldoc = newdoc;
    std_free((char*)url);
    return Ok;
}

Err cmd_eval(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);

    if ((rest = substr_match(line, "browse", 1))) { return cmd_browse(session, rest); }
    if ((rest = substr_match(line, "go", 1))) { return cmd_browse(session, rest); }
    if ((rest = substr_match(line, "url", 1))) { return cmd_set_url(session, rest); }

    if (!htmldoc_is_valid(session_current_doc(session)) ||!session->url_client) {
        return "no document";
    }

    if ((rest = substr_match(line, "ahref", 2))) { return cmd_ahre(session, rest); }
    if ((rest = substr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = substr_match(line, "class", 3))) { return "TODO: class"; }
    if ((rest = substr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = substr_match(line, "fetch", 1))) { return cmd_fetch(session); }
    if ((rest = substr_match(line, "input", 1))) { return cmd_input(session, rest); }
    if ((rest = substr_match(line, "summary", 1))) { return dbg_session_summary(session); }
    if ((rest = substr_match(line, "tag", 2))) { return cmd_tag(rest, session); }
    if ((rest = substr_match(line, "text", 2))) { return cmd_text(session); }

    return "unknown cmd";
}


Err process_line(Session session[static 1], const char* line) {
    if (!line) { session->quit = true; return "no input received, exiting"; }
    line = cstr_skip_space(line);

    if (!*line) { return Ok; }

    const char* rest = NULL;
    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return Ok;}
    if (*line == '/') { return "/ (search) not implemented"; }
    else if (*line == ':') {
        return ed_eval(session, line + 1);
    } else {
        return cmd_eval(session, line);
    }

    return "unexpected";
}
