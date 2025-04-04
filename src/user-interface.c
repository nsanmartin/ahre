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

#define define_err_cmd(Name, Rv) static inline Err Name(CmdParams p[static 1]){(void)p;return Rv;}

#define CMD_NO_PARAMS 0x1
#define CMD_CHAR 0x2
#define CMD_EMPTY 0x4

static Err cmd_echo (CmdParams p[static 1]) { 
    if (p->s && p->ln && *p->ln) session_write_msg(p->s, (char*)p->ln, strlen(p->ln));
    return Ok;
}
static Err cmd_quit (CmdParams p[static 1]) { session_quit_set(p->s); return Ok;}

static inline bool _char_cmd_match_(SessionCmd* cmd, CmdParams p[static 1]) {
    if (cmd->flags & CMD_CHAR && cmd->name[0] == p->ln[0]) {
        ++p->ln;
        return true;
    }
    return false;
}

static inline bool _empty_match_(SessionCmd* cmd, CmdParams p[static 1]) {
    return cmd->flags & CMD_EMPTY && !*cstr_skip_space(p->ln);
}

static inline bool _no_params_match_(SessionCmd* cmd, CmdParams p[static 1]) {
    return cmd->flags & CMD_NO_PARAMS && *cstr_skip_space(p->ln);
}

static inline bool _name_match_(SessionCmd* cmd, CmdParams p[static 1]) {
    const char* rest;
    if ((rest = csubstr_match(p->ln, cmd->name, cmd->match))) {
        p->ln = rest;
        return true;
    }
    return false;
}

static bool _is_help_cmd_end_(CmdParams p[static 1]) {
    const char* ln = cstr_skip_space(p->ln);
    return *ln == '?' && !*cstr_skip_space(ln+1);
}

Err run_cmd_help(Session* s, SessionCmd cmd[static 1]) {

    if (cmd->help) session_write_msg(s, (char*)cmd->help, strlen(cmd->help));
    if (cmd->subcmds) {
        session_write_msg_lit__(s, "sub commands:\n");
        for (SessionCmd* sub = cmd->subcmds; sub->name ; ++sub) {
            session_write_msg_lit__(s, "  ");
            session_write_msg(s, (char*)sub->name, strlen(sub->name));
            session_write_msg_lit__(s, "\n");
        }
    }
    return Ok;
}

Err run_cmd__(CmdParams p[static 1], SessionCmd cmdlist[]) {
    p->ln = cstr_skip_space(p->ln);
    for (SessionCmd* cmd = cmdlist; cmd->name ; ++cmd) {
        if (_char_cmd_match_(cmd, p)) {
            if (_is_help_cmd_end_(p)) return run_cmd_help(p->s, cmd);
            return cmd->fn(p);
        }
        else if (_name_match_(cmd, p)) {
            if (_is_help_cmd_end_(p)) return run_cmd_help(p->s, cmd);
            if (_no_params_match_(cmd, p)) return  "unexpected cmd param";
            else return cmd->fn(p);
        } else if (_empty_match_(cmd, p)) return cmd->fn(p);
    }
    return "invalid command";
}

/* commands */

Err cmd_tabs(CmdParams p[static 1]) {
    static SessionCmd _session_cmd_tabs_[] =
        { {.name="-", .fn=cmd_tabs_back, .help=NULL, .flags=CMD_CHAR}
        , {.name="", .fn=cmd_tabs_info, .help=NULL, .flags=CMD_EMPTY}
        , {0}
    };
    return run_cmd__(p, _session_cmd_tabs_);
}

static SessionCmd _cmd_doc_[] =
    { {.name="", .fn=cmd_doc_info,           .help=NULL,  .flags=CMD_EMPTY}
    , {.name="A", .fn=cmd_doc_A,              .help=NULL, .flags=CMD_CHAR}
    , {.name="+", .fn=cmd_doc_bookmark_add,   .help=NULL, .flags=CMD_CHAR}
    //, {.name="%", .fn=cmd_doc_dbg_traversal,  .help=NULL, .flags=CMD_CHAR}
    //TODO^, {.name="draw", .match=1, .fn=cmd_doc_draw, .help=NULL}
    //, {.name="hide", .match=1, .fn=cmd_doc_hide, .help=NULL}
    //, {.name="show", .match=1, .fn=cmd_doc_show, .help=NULL}
    , {0}
};
Err cmd_doc(CmdParams p[static 1]) {
    return run_cmd__(p, _cmd_doc_);
}

static SessionCmd _cmd_textbuf_[] =
    { {.name="",            .fn=cmd_textbuf_print,        .help=NULL, .flags=CMD_EMPTY}
    , {.name="g", .match=1, .fn=cmd_textbuf_global,       .help=NULL}
    , {.name="l", .match=1, .fn=cmd_textbuf_dbg_print_all_lines_nums, .help=NULL,
        .flags=CMD_NO_PARAMS}
    , {.name="n", .match=1, .fn=cmd_textbuf_print_n,      .help=NULL,.flags=CMD_NO_PARAMS}
    , {.name="print", .match=1, .fn=cmd_textbuf_print,    .help=NULL,.flags=CMD_NO_PARAMS}
    , {.name="write", .match=1, .fn=cmd_textbuf_write,    .help=NULL }
    , {0}
};

Err cmd_textbuf(CmdParams p[static 1]) {
    try( session_current_buf(p->s, &p->tb));
    try( cmd_parse_range(p->s, &p->r, &p->ln));
    return run_cmd__(p, _cmd_textbuf_);
}

Err cmd_sourcebuf(CmdParams p[static 1]) {
    try( session_current_src(p->s, &p->tb));
    try( cmd_parse_range(p->s, &p->r, &p->ln));
    return run_cmd__(p, _cmd_textbuf_);
}

Err cmd_anchor(CmdParams p[static 1]) {
    p->ln = cstr_skip_space(p->ln);
    long long unsigned linknum;
    try( parse_base36_or_throw(&p->ln, &linknum));
    p->ln = cstr_skip_space(p->ln);
    switch (*p->ln) {
        case '\'': return cmd_anchor_print(p->s, (size_t)linknum); 
        case '\0': 
        case '*': return cmd_anchor_asterisk(p->s, (size_t)linknum);
        default: return "?";
    }
}

Err cmd_input(CmdParams p[static 1]) {
    p->ln = cstr_skip_space(p->ln);
    long long unsigned linknum;
    try( parse_base36_or_throw(&p->ln, &linknum));
    p->ln = cstr_skip_space(p->ln);
    switch (*p->ln) {
        case '\'': return cmd_input_print(p->s, linknum);
        case '\0': 
        case '*': return cmd_input_submit_ix(p->s, linknum);
        case '=': return cmd_input_ix(p->s, linknum, p->ln + 1); 
        default: return "?";
    }
}

Err cmd_image(CmdParams p[static 1]) {
    p->ln = cstr_skip_space(p->ln);
    long long unsigned linknum;
    try( parse_base36_or_throw(&p->ln, &linknum));
    p->ln = cstr_skip_space(p->ln);
    switch (*p->ln) {
        case '\'': return cmd_image_print(p->s, linknum);
        default: return "?";
    }
    return Ok;
}

Err cmd_set_session_input(CmdParams p[static 1]);
Err cmd_set_session_winsz(CmdParams p[static 1]);
Err cmd_set_session_ncols(CmdParams p[static 1]);
Err cmd_set_session_monochrome(CmdParams p[static 1]);
Err cmd_set_curl(CmdParams p[static 1]);
define_err_cmd(cmd_set_session_invalid_option, "not a session option")

static SessionCmd _cmd_set_session_[] = 
    { {.name="input",        .match=1, .fn=cmd_set_session_input,      .help=NULL}
    , {.name="monochrome",   .match=1, .fn=cmd_set_session_monochrome, .help=NULL}
    , {.name="ncols",        .match=1, .fn=cmd_set_session_ncols,      .help=NULL}
    , {.name="winsz",        .match=1, .fn=cmd_set_session_winsz,      .help=NULL}
    , {0}
    };
Err cmd_set_session(CmdParams p[static 1]) { return run_cmd__(p, _cmd_set_session_); }

//define_err_cmd(cmd_set_invalid_option, "not an option")

Err cmd_set(CmdParams p[static 1]) {
    static SessionCmd _session_cmd_set_[] = 
        { {.name="curl",    .match=1, .fn=cmd_set_curl,    .help=NULL}
        , {.name="session", .match=1, .fn=cmd_set_session, .help=NULL,.subcmds=_cmd_set_session_}
        , {0}
        };
    return run_cmd__(p, _session_cmd_set_);
}

Err dbg_print_form(CmdParams p[static 1]) ;
Err cmd_bookmarks(CmdParams p[static 1]);


Err _main_help_fn_(CmdParams p[static 1]);

#define CMD_HELP_IX 14
static SessionCmd _session_cmd_[] = 
    { [0]={.name="bookmarks", .match=1, .fn=cmd_bookmarks, .help=NULL}
    , [1]={.name="cookies",   .match=1, .fn=cmd_cookies,   .help=NULL}
    , [2]={.name="echo",      .match=1, .fn=cmd_echo,      .help=NULL}
    , [3]={.name="go",        .match=1, .fn=cmd_open_url,  .help=NULL}
    , [4]={.name="quit",      .match=1, .fn=cmd_quit,      .help=NULL, .flags=CMD_NO_PARAMS}
    , [5]={.name="set",       .match=1, .fn=cmd_set,       .help=NULL }
    , [6]={.name="|",             .fn=cmd_tabs,       .help=NULL, .flags=CMD_CHAR}
    , [7]={.name=".",             .fn=cmd_doc,        .help=NULL, .flags=CMD_CHAR, .subcmds=_cmd_doc_}
    , [8]={.name=":",             .fn=cmd_textbuf,    .help=NULL, .flags=CMD_CHAR, .subcmds=_cmd_textbuf_}
    , [9]={.name="<",             .fn=cmd_sourcebuf,  .help=NULL, .flags=CMD_CHAR, .subcmds=_cmd_textbuf_}
    , [10]={.name="&",            .fn=dbg_print_form, .help=NULL, .flags=CMD_CHAR}
    , [11]={.name=ANCHOR_OPEN_STR,.fn=cmd_anchor,     .help=NULL, .flags=CMD_CHAR}
    , [12]={.name=INPUT_OPEN_STR, .fn=cmd_input,      .help=NULL, .flags=CMD_CHAR}
    , [13]={.name=IMAGE_OPEN_STR, .fn=cmd_image,      .help=NULL, .flags=CMD_CHAR}
    , [CMD_HELP_IX]={.name="?",   .fn=_main_help_fn_, .help=NULL, .flags=CMD_CHAR, .subcmds=_session_cmd_}
    , [15]={0}
    };

Err process_line(Session session[static 1], const char* line) {

    if (!line) { session_quit_set(session); return "no input received, exiting"; }
    line = cstr_skip_space(line);
    if (*line == '\\') line = cstr_skip_space(line + 1);
    if (!*line) { return Ok; }
    return run_cmd__(&(CmdParams){.s=session,.ln=line}, _session_cmd_);
}

Err _main_help_fn_(CmdParams p[static 1]) {
    return run_cmd_help(p->s, &_session_cmd_[CMD_HELP_IX]);
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
