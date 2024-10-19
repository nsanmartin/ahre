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


//TODO: move this to a method
Err cmd_set_url(Session session[static 1], const char* url) {
    Str u;
    if(str_init(&u, url)) { return "bad url"; }
    str_trim_space(&u);
    if (!str_is_empty(&u) && (!session->htmldoc->url || strncmp(u.s, session->htmldoc->url, u.len))) {
        htmldoc_cleanup(session->htmldoc);
        if (htmldoc_init(session->htmldoc, &u)) {
            return "could not init htmldoc";
        }

        HtmlDoc* htmldoc = session_current_doc(session);
        if (htmldoc_has_url(htmldoc)) {
            Err err = htmldoc_fetch(htmldoc, session->url_client);
            if (err) {
                return err_fmt("\nerror fetching url: '%s': %s", url, err);
            }
        }
        printf("set with url: %s\n", url);
    } else {
       printf("url: %s\n",  session->htmldoc->url ? session->htmldoc->url : "<no url>");
    }
    return Ok;
}


Err cmd_eval(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);

    if ((rest = substr_match(line, "url", 1))) { return cmd_set_url(session, rest); }

    if (!htmldoc_is_valid(session_current_doc(session)) ||!session->url_client) {
        return "no document";
    }

    if ((rest = substr_match(line, "ahref", 2)) && !*rest) { return cmd_ahre(session); }
    if ((rest = substr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = substr_match(line, "class", 3))) { return "TODO: class"; }
    if ((rest = substr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = substr_match(line, "fetch", 1))) { return cmd_fetch(session); }
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
    if (*line == '\\') {
        return cmd_eval(session, line + 1);
    } else {
        return ed_eval(session_current_buf(session), line);
    }

    return "unexpected";
}
