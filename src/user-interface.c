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

#define define_err_cmd(Name, Rv) static inline Err Name(Session s[static 1], const char* ln) {\
    (void)s; (void)ln; return Rv; }

#define CMD_NO_PARAMS 0x1
#define CMD_CHAR 0x2
#define CMD_EMPTY 0x4

typedef Err (*SessionCmdTextBufFn)
    (Session s[static 1], TextBuf tb[static 1], Range* r, const char* rest);
typedef struct {
    const char* name;
    size_t len;
    size_t match;
    SessionCmdTextBufFn fn;
    unsigned flags;
    const char* help;
} SessionCmdTextBuf ;

typedef Err (*SessionCmdFn)(Session s[static 1], const char* rest);
typedef struct {
    const char* name;
    size_t len;
    size_t match;
    SessionCmdFn fn;
    unsigned flags;
    const char* help;
} SessionCmd ;

static Err cmd_echo (Session s[static 1], const char* rest) { (void)s; puts(rest); return Ok; }
static Err
cmd_quit (Session s[static 1], const char* rest) { (void)rest; session_quit_set(s); return Ok;}
#define _char_cmd_match_(Scmd, Ln) ((Scmd)->flags & CMD_CHAR && (Scmd)->name[0] == (Ln)[0])
#define _empty_match_(Scmd, Ln) ((Scmd)->flags & CMD_EMPTY && !*cstr_skip_space((Ln)))
#define _no_params_match_(Scmd, Ln) ((Scmd)->flags & CMD_NO_PARAMS && *cstr_skip_space((Ln)))

Err run_cmd_help(Session s[static 1], const char* ln, SessionCmd cmdlist[]) {
    (void)s; (void)ln; (void)cmdlist;
    ln = cstr_skip_space(ln);
    if (!*ln) {

        session_write_msg_lit__(s, "sub commands:\n");
        for (SessionCmd* cmd = cmdlist; cmd->name ; ++cmd) {
            session_write_msg_lit__(s, "  ");
            session_write_msg(s, (char*)cmd->name, strlen(cmd->name));
            session_write_msg_lit__(s, "\n");
        }
    }
    return Ok;
}

Err
run_cmd__(Session s[static 1], const char* line, SessionCmd cmdlist[], SessionCmdFn default_cmd) {
    const char* rest = NULL;
    line = cstr_skip_space(line);
    if (*line == '?') return run_cmd_help(s, line + 1, cmdlist);
    for (SessionCmd* cmd = cmdlist; cmd->name ; ++cmd) {
        if (_char_cmd_match_(cmd, line)) return cmd->fn(s, cstr_skip_space(line+1));
        else if ((rest = csubstr_match(line, cmd->name, cmd->match))) {
            if (_no_params_match_(cmd, rest)) return  "unexpected cmd param";
            else return cmd->fn(s, rest);
        } else if (_empty_match_(cmd, line)) return cmd->fn(s, line);
    }
    return default_cmd(s, line);
}

Err run_cmd_textbuf__(
    Session s[static 1],
    const char* line,
    SessionCmdTextBuf cmdlist[],
    SessionCmdTextBufFn default_cmd,
    TextBuf tb[static 1],
    Range* r
) {
    const char* rest = NULL;
    line = cstr_skip_space(line);
    for (SessionCmdTextBuf* cmd = cmdlist; cmd->name ; ++cmd) {
        if (_char_cmd_match_(cmd, line)) return cmd->fn(s, tb, r, cstr_skip_space(line+0));
        else if ((rest = csubstr_match(line, cmd->name, cmd->match))) {
            if (_no_params_match_(cmd, rest)) return  "unexpected textbuf cmd param";
            else return cmd->fn(s, tb, r, rest);
        } else if (_empty_match_(cmd, line)) return cmd->fn(s, tb, r, line);
    }
    return default_cmd(s, tb, r, line);
}

/* commands */

Err cmd_tabs(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_tabs_[] =
        { {.name="-", .fn=cmd_tabs_back, .help=NULL, .flags=CMD_CHAR}
        , {.name="", .fn=cmd_tabs_info, .help=NULL, .flags=CMD_EMPTY}
        , {0}
    };
    return run_cmd__(session, line, _session_cmd_tabs_, cmd_tabs_goto);
}

Err cmd_doc(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_doc_[] =
        { {.name="", .fn=cmd_doc_info,           .help=NULL,  .flags=CMD_EMPTY}
        , {.name="A", .fn=cmd_doc_A,              .help=NULL, .flags=CMD_CHAR}
        , {.name="+", .fn=cmd_doc_bookmark_add,   .help=NULL, .flags=CMD_CHAR}
        , {.name="%", .fn=cmd_doc_dbg_traversal,  .help=NULL, .flags=CMD_CHAR}
        , {.name="draw", .match=1, .fn=cmd_doc_draw, .help=NULL}
        , {.name="hide", .match=1, .fn=cmd_doc_hide, .help=NULL}
        , {.name="show", .match=1, .fn=cmd_doc_show, .help=NULL}
        , {0}
    };
    return run_cmd__(session, line, _session_cmd_doc_, cmd_unknown);
}

static SessionCmdTextBuf _session_cmd_doc_[] =
    { {.name="",            .fn=cmd_textbuf_print,        .help=NULL, .flags=CMD_EMPTY}
    , {.name="g", .match=1, .fn=cmd_textbuf_global,       .help=NULL}
    , {.name="l", .match=1, .fn=cmd_textbuf_dbg_print_all_lines_nums, .help=NULL,
        .flags=CMD_NO_PARAMS}
    , {.name="n", .match=1, .fn=cmd_textbuf_print_n,      .help=NULL,.flags=CMD_NO_PARAMS}
    , {.name="print", .match=1, .fn=cmd_textbuf_print,    .help=NULL,.flags=CMD_NO_PARAMS}
    , {.name="write", .match=1, .fn=cmd_textbuf_write,    .help=NULL }
    , {0}
};

Err cmd_textbuf(Session s[static 1], const char* line) {
    TextBuf* textbuf;
    try( session_current_buf(s, &textbuf));
    Range range;
    try( cmd_parse_range(s, &range, &line));
    return run_cmd_textbuf__(s, line, _session_cmd_doc_, cmd_textbuf_unknown, textbuf, &range);
}

Err cmd_sourcebuf(Session s[static 1], const char* line) {
    TextBuf* textbuf;
    try( session_current_src(s, &textbuf));
    Range range;
    try( cmd_parse_range(s, &range, &line));
    return run_cmd_textbuf__(s, line, _session_cmd_doc_, cmd_textbuf_unknown, textbuf, &range);
}

Err cmd_anchor(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    switch (*line) {
        case '\'': return cmd_anchor_print(session, (size_t)linknum); 
        case '\0': 
        case '*': return cmd_anchor_asterisk(session, (size_t)linknum);
        default: return "?";
    }
}

Err cmd_input(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    switch (*line) {
        case '\'': return cmd_input_print(session, linknum);
        case '\0': 
        case '*': return cmd_input_submit_ix(session, linknum);
        case '=': return cmd_input_ix(session, linknum, line + 1); 
        default: return "?";
    }
}

Err cmd_image(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    switch (*line) {
        case '\'': return cmd_image_print(session, linknum);
        default: return "?";
    }
    return Ok;
}

Err cmd_set_session_input(Session session[static 1], const char* line);
Err cmd_set_session_winsz(Session session[static 1], const char* ln);
Err cmd_set_session_ncols(Session session[static 1], const char* line);
Err cmd_set_session_monochrome(Session session[static 1], const char* line);
Err cmd_set_curl(Session session[static 1], const char* line);
define_err_cmd(cmd_set_session_invalid_option, "not a session option")

Err cmd_set_session(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_set_session_[] = 
        { {.name="input",        .match=1, .fn=cmd_set_session_input,      .help=NULL}
        , {.name="monochrome",   .match=1, .fn=cmd_set_session_monochrome, .help=NULL}
        , {.name="ncols",        .match=1, .fn=cmd_set_session_ncols,      .help=NULL}
        , {.name="winsz",        .match=1, .fn=cmd_set_session_winsz,      .help=NULL}
        , {0}
        };
    return run_cmd__(session, line, _session_cmd_set_session_, cmd_set_session_invalid_option);
}

define_err_cmd(cmd_set_invalid_option, "not an option")

Err cmd_set(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_set_[] = 
        { {.name="curl",    .match=1, .fn=cmd_set_curl,    .help=NULL}
        , {.name="session", .match=1, .fn=cmd_set_session, .help=NULL}
        , {0}
        };
    return run_cmd__(session, line, _session_cmd_set_, cmd_set_invalid_option);
}

Err process_line(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_[] = 
        { {.name="bookmarks", .match=1, .fn=cmd_bookmarks, .help=NULL}
        , {.name="cookies",   .match=1, .fn=cmd_cookies,   .help=NULL}
        , {.name="echo",      .match=1, .fn=cmd_echo,      .help=NULL}
        , {.name="go",        .match=1, .fn=cmd_open_url,  .help=NULL}
        , {.name="quit",      .match=1, .fn=cmd_quit,      .help=NULL, .flags=CMD_NO_PARAMS}
        , {.name="set",       .match=1, .fn=cmd_set,       .help=NULL }
        , {.name="|",             .fn=cmd_tabs,       .help=NULL, .flags=CMD_CHAR}
        , {.name=".",             .fn=cmd_doc,        .help=NULL, .flags=CMD_CHAR}
        , {.name=":",             .fn=cmd_textbuf,    .help=NULL, .flags=CMD_CHAR}
        , {.name="<",             .fn=cmd_sourcebuf,  .help=NULL, .flags=CMD_CHAR}
        , {.name="&",             .fn=dbg_print_form, .help=NULL, .flags=CMD_CHAR}
        , {.name=ANCHOR_OPEN_STR, .fn=cmd_anchor,     .help=NULL, .flags=CMD_CHAR}
        , {.name=INPUT_OPEN_STR,  .fn=cmd_input,      .help=NULL, .flags=CMD_CHAR}
        , {.name=IMAGE_OPEN_STR,  .fn=cmd_image,      .help=NULL, .flags=CMD_CHAR}
        , {0}
        };

    if (!line) { session_quit_set(session); return "no input received, exiting"; }
    line = cstr_skip_space(line);
    if (*line == '\\') line = cstr_skip_space(line + 1);
    if (!*line) { return Ok; }
    return run_cmd__(session, line, _session_cmd_, cmd_misc);
}

Err process_line_line_mode(Session* s, const char* line) {
    if (!s) return "error: no session :./";
    return process_line(s, line);
}

Err process_line_vi_mode(Session* s, const char* line) {
    if (!s) return "error: no session :./";
    try( process_line(s, line));
    return cmd_doc_draw(s, "");//TODO: do not rewrite the title
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
