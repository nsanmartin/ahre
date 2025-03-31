#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bookmark.h"
#include "cmd.h"
#include "constants.h"
#include "debug.h"
#include "mem.h"
#include "ranges.h"
#include "url.h"
#include "user-input.h"
#include "user-out.h"
#include "user-interface.h"
#include "utils.h"
#include "readpass.h"
#include "wrapper-lexbor-curl.h"


Err read_line_from_user(Session session[static 1]) {
    char* line = 0x0;
    try( session_uinput(session)->read(session, NULL, &line));
    Err err = process_line(session, line);
    std_free(line);
    return err;
}


typedef Err (*SessionCmdFn)(Session s[static 1], const char* rest);
typedef struct {
    const char* name;
    size_t len;
    size_t match;
    bool empty_rest;
    SessionCmdFn fn;
    const char* help;
} SessionCmd ;

static Err cmd_echo (Session s[static 1], const char* rest) { (void)s; puts(rest); return Ok; }
static Err
cmd_quit (Session s[static 1], const char* rest) { (void)rest; session_quit_set(s); return Ok;}

SessionCmd _session_cmd_[] = 
    { {.name="bookmarks", .match=1, .fn=cmd_bookmarks, .help=NULL}
    , {.name="cookies",   .match=1, .fn=cmd_cookies,   .help=NULL}
    , {.name="echo",      .match=1, .fn=cmd_echo,      .help=NULL}
    , {.name="go",        .match=1, .fn=cmd_open_url,  .help=NULL}
    , {.name="quit",      .match=1, .fn=cmd_quit,      .help=NULL, .empty_rest=1,}
    , {.name="set",       .match=1, .fn=cmd_set,       .help=NULL }
    , {0}
    };


bool _run_cmd_(Session s[static 1], const char* line, SessionCmd cmdlist[], Err err[static 1]) {
    const char* rest = NULL;
    for (SessionCmd* cmd = cmdlist; cmd->name ; ++cmd) {
        if ((rest = csubstr_match(line, cmd->name, cmd->match))) {
            *err = cmd->fn(s, rest);
            return true;
        }
    }
    return false;
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
    Err err = Ok;
    if (_run_cmd_(session, line, _session_cmd_, &err)) return err;
    //if ((rest = csubstr_match(line, "bookmarks", 1))) { return cmd_bookmarks(session, rest); }
    //if ((rest = csubstr_match(line, "cookies", 1))) { return cmd_cookies(session, rest); }
    ////TODO@ if ((rest = csubstr_match(line, "dbg", 1))) return cmd_dbg(session, rest)
    //if ((rest = csubstr_match(line, "echo", 1))) return puts(rest) < 0 ? "error: puts failed" : Ok;
    //if ((rest = csubstr_match(line, "go", 1))) { return cmd_open_url(session, rest); }
    //if ((rest = csubstr_match(line, "quit", 1)) && !*rest) { session_quit_set(session); return Ok;}
    //if ((rest = csubstr_match(line, "set", 1))) { return cmd_set(session, rest); }

    if (*line == '|') return cmd_tabs(session, line + 1);
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    if (!htmldoc_is_valid(htmldoc) ||!session->url_client) return "no document";

    if (*line == '.') return cmd_doc(htmldoc, line + 1, session);
    //TODO: implement search in textbuf
    if (*line == '/') return "to search in buffer use ':/' (not just '/')";

    //TODO: obtain range from line and pase it already parsed to eval fn

    if ((rest = csubstr_match(line, "draw", 1))) { return cmd_draw(session, rest); }

    TextBuf* tb;
    try( session_current_buf(session, &tb));
    if (*line == ':') return cmd_textbuf(session, tb, line + 1);
    if (*line == '<') return cmd_textbuf(session, htmldoc_sourcebuf(htmldoc), line + 1);
    if (*line == '&') return dbg_print_form(session, line + 1);//TODO: form is &Form or something else
    if (*line == ANCHOR_OPEN_STR[0]) return cmd_anchor(session, line + 1);
    if (*line == INPUT_OPEN_STR[0]) return cmd_input(session, line + 1);
    if (*line == IMAGE_OPEN_STR[0]) return cmd_image_eval(session, line + 1);

    return cmd_misc(session, line);
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
