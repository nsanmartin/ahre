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
    if (!*url) return err_fmt("%s", htmldoc->url);
    Str u;
    if(str_init(&u, url)) { return err_fmt("error: init str '%s'", url); }
    if (htmldoc->url && strncmp(u.s, htmldoc->url, u.len) == 0) 
        return "same url, not updating";

    if (cstr_starts_with(u.s, "//"))
        str_prepend(&u, "https://"); ///TODO: suppoert for http
    else if (cstr_starts_with(u.s, "/"))
        str_prepend(&u, htmldoc->url);
    htmldoc_cleanup(htmldoc);
    if (htmldoc_init(htmldoc, &u)) {
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


Err cmd_go(Session session[static 1], const char* link) {
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
    if (!newdoc) return "error: could not create html doc";
    if (newdoc->url && newdoc->lxbdoc) {
        if ((e = htmldoc_fetch(newdoc, session_url_client(session)))) {
            return e;
        }

        if ((e = htmldoc_browse(newdoc))) {
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
    if ((rest = substr_match(line, "url", 1))) { return cmd_set_url(session, rest); }

    if (!htmldoc_is_valid(session_current_doc(session)) ||!session->url_client) {
        return "no document";
    }

    if ((rest = substr_match(line, "ahref", 2)) && !*rest) { return cmd_ahre(session); }
    if ((rest = substr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = substr_match(line, "class", 3))) { return "TODO: class"; }
    if ((rest = substr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = substr_match(line, "fetch", 1))) { return cmd_fetch(session); }
    if ((rest = substr_match(line, "go", 1))) { return cmd_go(session, rest); }
    if ((rest = substr_match(line, "summary", 1))) { return dbg_session_summary(session); }
    if ((rest = substr_match(line, "tag", 2))) { return cmd_tag(rest, session); }
    if ((rest = substr_match(line, "text", 2))) { return cmd_text(session); }

    return "unknown lxb cmd";
}


Err process_line(Session session[static 1], const char* line) {
    if (!line) { session->quit = true; return "no input received, exiting"; }
    line = cstr_skip_space(line);

    if (!*line) { return Ok; }

    const char* rest = NULL;
    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return Ok;}
    if (*line == '/') { return "/ (search) not implemented"; }
    else if (*line == '\\') {
        return cmd_eval(session, line + 1);
    } else {
        return ed_eval(session, line);
    }

    return "unexpected";
}
