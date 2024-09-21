#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <ah/debug.h>
#include <ah/mem.h>
#include <ah/ranges.h>
#include <ah/user-cmd.h>
#include <ah/user-interface.h>
#include <ah/utils.h>
#include <ah/wrappers.h>


int read_line_from_user(Session session[static 1]) {
    char* line = 0x0;
    line = readline("");
    session->user_line_callback(session, line);
    add_history(line);
    destroy(line);
    return 0;
}

const char* substr_match(const char* s, const char* cmd, size_t len) {
    if (!*s) { return 0x0; }
	for (; *s && !isspace(*s); ++s, ++cmd, (len?--len:len)) {
		if (*s != *cmd) { return 0x0; }
	}
    if (len) { 
        printf("...%s?\n", cmd);
        return 0x0;
    }
	return skip_space(s);
}

bool substr_match_all(const char* s, size_t len, const char* cmd) {
    return (s=substr_match(s, cmd, len)) && !*skip_space(s);
}


int cmd_set_url(Session session[static 1], const char* url) {
    puts("url");
    //Str u = (Str){.s=url, .len=strlen(url)};
    Str u;
    if(str_init(&u, url)) { return -1; }
    trim_space(&u);
    if (!str_is_empty(&u) && (!session->doc->url || strncmp(u.s, session->doc->url, u.len))) {
        doc_cleanup(session->doc);
        if (doc_init(session->doc, &u)) {
            puts("Doc init error");
            return -1;
        }

        Doc* lxbdoc = session_current_doc(session);
        if (doc_has_url(lxbdoc)) {
            ErrStr err = doc_fetch(session->url_client, lxbdoc);
            if (err) {
                puts("TODO:cleanup");
                fprintf(stderr, "error fetching url: %s\n", err);
                return -1;
            }
        }
        printf("set with url: %s\n", url);
    } else {
       printf("url: %s\n",  session->doc->url ? session->doc->url : "<no url>");
    }
    return 0;
}


int ahcmd(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = skip_space(line);

    if ((rest = substr_match(line, "url", 1))) { return cmd_set_url(session, rest); }

    if (!session->doc->url || !session->doc->url || !session->url_client) {
        ah_log_info("no document"); return 0;
    }

    if ((rest = substr_match(line, "ahref", 2)) && !*rest) { return cmd_ahre(session); }
    if ((rest = substr_match(line, "attr", 2))) { puts("TODO: attr"); }
    if ((rest = substr_match(line, "class", 3))) { puts("TODO: class"); }
    if ((rest = substr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = substr_match(line, "fetch", 1))) { return cmd_fetch(session); }
    if ((rest = substr_match(line, "summary", 1))) { return dbg_session_summary(session); }
    if ((rest = substr_match(line, "tag", 2))) { return cmd_tag(rest, session); }
    if ((rest = substr_match(line, "text", 2))) { return cmd_text(session); }

    return 0;
}

int ed_eval(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    if (!line) { return 0; }
    Range range = {0};
    line = parse_range(line, session, &range);
    if (!line) {
        puts("invalid range");
        return 0;
    } else if (range.end == 0) range.end = range.beg; 

    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return 0;}
    if (textbuf_is_empty(session_current_buf(session))) { 
        puts("empty buffer");
        return 0;
    }

    printf("range: %lu, %lu\n", range.beg, range.end);
    session_current_buf(session)->current_line = range.end;

    if ((rest = substr_match(line, "a", 1)) && !*rest) { return ed_print_all(session); }
    if ((rest = substr_match(line, "l", 1)) && !*rest) { return dbg_print_all_lines_nums(session); }
    if ((rest = substr_match(line, "g", 1)) && *rest) { return ed_global(session, rest); }
    if ((rest = substr_match(line, "print", 1)) && !*rest) { return ed_print(session, &range); }
    if ((rest = substr_match(line, "write", 1))) { return ed_write(rest, session); }
    puts("unknown command");
    return 0;
}

int process_line(Session session[static 1], const char* line) {
    if (!line) { session->quit = true; return 0; }
    line = skip_space(line);

    if (!*line) { return 0; }

    if (*line == '\\') {
        if (ahcmd(session, line + 1)) { return -1; }

    } else {
        return ed_eval(session, line);
    }

    return 0;
}
