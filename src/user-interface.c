#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
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

#define CMD_NO_PARAMS 0x1
#define CMD_CHAR 0x2
#define CMD_EMPTY 0x4
#define CMD_ANY 0x8

#define CMD_ECHO_DOC \
    "Prints in the message area the received parameters.\n"

static Err cmd_echo (CmdParams p[static 1]) { 
    if (p->s && p->ln && *p->ln) {
        session_write_msg(p->s, (char*)p->ln, strlen(p->ln));
        session_write_msg_lit__(p->s, "\n");
    }
    return Ok;
}

#define CMD_QUIT_DOC "Quits ahre.\n"
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
static inline bool _any_match_(SessionCmd* cmd) { return cmd->flags & CMD_ANY; }

static const char* _name_match_impl_(SessionCmd* cmd, CmdParams p[static 1]) {
    const char* s = p->ln;
    const char* name = cmd->name;
    size_t len = cmd->len;
    if (!*s || !isalpha(*s)) { return 0x0; }
	for (; *s && isalpha(*s); ++s, ++name, (len?--len:len)) {
		if (*s != *name) { return 0x0; }
	}
    if (len) { 
        try(session_write_msg_lit__(p->s, "..."));
        try(session_write_msg(p->s, (char*)cmd->name, strlen(cmd->name)));
        try(session_write_msg_lit__(p->s, "?\n"));
        return 0x0;
    }
	return cstr_skip_space(s);
}
static inline bool _name_match_(SessionCmd* cmd, CmdParams p[static 1]) {
    const char* rest;
    if ((rest = _name_match_impl_(cmd,p))) {
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
#define RUN_CMD_DOC_TODO_MSG \
    "Not documented command.\n"\
    "TODO: document it.\n"
    if (!cmd->help && !cmd->subcmds) 
        session_write_msg(s, RUN_CMD_DOC_TODO_MSG, lit_len__(RUN_CMD_DOC_TODO_MSG));
    if (cmd->help) session_write_msg(s, (char*)cmd->help, strlen(cmd->help));
    if (cmd->subcmds) {
        session_write_msg_lit__(s, "sub commands:\n");
        for (SessionCmd* sub = cmd->subcmds; sub->name ; ++sub) {
            if (sub->flags & CMD_CHAR) {
                session_write_msg_lit__(s, "  '");
                session_write_msg(s, (char*)sub->name, strlen(sub->name));
                session_write_msg_lit__(s, "'\n");
            } else if (sub->flags & CMD_EMPTY) {
                session_write_msg_lit__(s, "  ");
                session_write_msg_lit__(s, "<empty>");
                session_write_msg_lit__(s, "\n");
            } else if (sub->flags & CMD_ANY) {
                session_write_msg_lit__(s, "  ");
                session_write_msg_lit__(s, "<any> ");
                session_write_msg(s, (char*)sub->name, strlen(sub->name));
                session_write_msg_lit__(s, "\n");
            }
        }
    }
    return Ok;
}

Err run_cmd__(CmdParams p[static 1], SessionCmd cmdlist[]) {
    p->ln = cstr_skip_space(p->ln);
    for (SessionCmd* cmd = cmdlist; cmd->name ; ++cmd) {
        if (_any_match_(cmd)) return cmd->fn(p);
        if (_char_cmd_match_(cmd, p)) {
            if (_is_help_cmd_end_(p)) return run_cmd_help(p->s, cmd);
            return cmd->fn(p);
        } else if (_name_match_(cmd, p)) {
            if (_is_help_cmd_end_(p)) return run_cmd_help(p->s, cmd);
            if (_no_params_match_(cmd, p)) return  "unexpected cmd param";
            else return cmd->fn(p);
        } else if (_empty_match_(cmd, p)) return cmd->fn(p);
    }
    //return "invalid command";
    return err_fmt("invalid command: %s", p->ln);
}

Err run_cmd_on_ix__(CmdParams p[static 1], SessionCmd cmdlist[]) {
    p->ln = cstr_skip_space(p->ln);
    Err parse_failed = parse_size_t_or_throw(&p->ln, &p->ix, 36);
    p->ln = cstr_skip_space(p->ln);
    /* we assume SIZE_MAX is not an index and is used as not id given */
    if (parse_failed) p->ix = SIZE_MAX; 
    //else return run_cmd__(p, cmdlist);
    return run_cmd__(p, cmdlist);
}
/* commands */

static SessionCmd _cmd_tabs_[] =
    { {.name="-", .fn=cmd_tabs_back, .help=NULL, .flags=CMD_CHAR}
    , {.name="",  .fn=cmd_tabs_info, .help=NULL, .flags=CMD_EMPTY}
    , {.name="goto",  .fn=cmd_tabs_goto, .help=NULL, .flags=CMD_ANY}
    , {0}
};
#define CMD_TABS_DOC "ahre tabs are tres of the visited urls.\n"
Err cmd_tabs(CmdParams p[static 1]) { return run_cmd__(p, _cmd_tabs_); }

static SessionCmd _cmd_doc_[] =
    { {.name="",     .fn=cmd_doc_info,           .help=CMD_DOC_INFO_DOC,     .flags=CMD_EMPTY}
    , {.name="\"",   .fn=cmd_doc_info,           .help=CMD_DOC_INFO_DOC,     .flags=CMD_CHAR}
    , {.name="A",    .fn=cmd_doc_A,              .help=NULL,                 .flags=CMD_CHAR}
    , {.name="+",    .fn=cmd_doc_bookmark_add,   .help=CMD_DOC_BOOKMARK_ADD, .flags=CMD_CHAR}
    , {.name="draw", .match=1, .fn=cmd_doc_draw, .help=CMD_DOC_DRAW}
    //, {.name="%", .fn=_cmd_doc_dbg_traversal,  .help=NULL, .flags=CMD_CHAR}
    //, {.name="hide", .match=1, .fn=_cmd_doc_hide, .help="Hide <ul>"}
    //, {.name="show", .match=1, .fn=_cmd_doc_show, .help="unhide <ul>"}
    , {0}
};

#define CMD_DOC_DOC \
    "'.' command: commands related to the current document\n"
Err cmd_doc(CmdParams p[static 1]) { return run_cmd__(p, _cmd_doc_); }

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
    try( _cmd_parse_range(p->s, &p->r, &p->ln));
    return run_cmd__(p, _cmd_textbuf_);
}

Err cmd_sourcebuf(CmdParams p[static 1]) {
    try( session_current_src(p->s, &p->tb));
    try( _cmd_parse_range(p->s, &p->r, &p->ln));
    return run_cmd__(p, _cmd_textbuf_);
}

Err cmd_anchor_print(CmdParams p[static 1]) { return _cmd_anchor_print(p->s, p->ix); }
Err cmd_anchor_asterisk(CmdParams p[static 1]) { return _cmd_anchor_asterisk(p->s, p->ix); }

static SessionCmd _cmd_anchor_[] =
    { {.name="\"", .fn=cmd_anchor_print,    .help=NULL, .flags=CMD_CHAR}
    , {.name="",   .fn=cmd_anchor_asterisk, .help=NULL, .flags=CMD_EMPTY}
    , {.name="*",  .fn=cmd_anchor_asterisk, .help=NULL, .flags=CMD_CHAR}
    , {0}
    };

#define CMD_ANCHOR_DOC \
    "[ LINK_ID [SUB_COMMAND]\n\n"\
    "'[' commands are applied to the links present in the document.\n"
Err cmd_anchor(CmdParams p[static 1]) { return run_cmd_on_ix__(p, _cmd_anchor_); }

static Err cmd_input_info(CmdParams p[static 1]) { return _cmd_input_print(p->s, p->ix); }
static Err cmd_input_submit(CmdParams p[static 1]) { return _cmd_input_submit_ix(p->s, p->ix); }
#define CMD_INPUT_SET \
    "{= [VALUE]\n\n"\
    "If VALUE is given, sets the value of an input element.\n"\
    "If not, user's input is hidden (useful for passwords).\n"
static Err cmd_input_set(CmdParams p[static 1]) { return _cmd_input_ix(p->s, p->ix, p->ln); }

static SessionCmd _cmd_input_[] =
    { {.name="\"", .fn=cmd_input_info,   .help=NULL, .flags=CMD_CHAR}
    , {.name="",   .fn=cmd_input_submit, .help=NULL, .flags=CMD_EMPTY}
    , {.name="*",  .fn=cmd_input_submit, .help=NULL, .flags=CMD_CHAR}
    , {.name="=",  .fn=cmd_input_set,   .help=CMD_INPUT_SET, .flags=CMD_CHAR}
    , {0}
    };

#define CMD_INPUT_DOC \
    "{ LINK_ID [SUB_COMMAND]\n\n"\
    "'{' commands are applied to the input elements present in the document.\n"
Err cmd_input(CmdParams p[static 1]) { return run_cmd_on_ix__(p, _cmd_input_); }

static Err cmd_image_info(CmdParams p[static 1]) { return _cmd_image_print(p->s, p->ix); }
static SessionCmd _cmd_image_[] =
    { {.name="\"", .fn=cmd_image_info,  .help=NULL, .flags=CMD_CHAR}
    , {0}
    };
#define CMD_IMAGE_DOC \
    "( LINK_ID [SUB_COMMAND]\n\n"\
    "'(' commands are applied to the images present in the document.\n"
Err cmd_image(CmdParams p[static 1]) {
    return run_cmd_on_ix__(p, _cmd_image_);
}

Err cmd_set_session_input(CmdParams p[static 1]);
Err cmd_set_session_winsz(CmdParams p[static 1]);
Err cmd_set_session_ncols(CmdParams p[static 1]);
Err cmd_set_session_monochrome(CmdParams p[static 1]);
#define CMD_SET_CURL "Set a curl option.\n"\
    "TODO: enumerate all available options.\n"
Err cmd_set_curl(CmdParams p[static 1]);

static SessionCmd _cmd_set_session_[] = 
    { {.name="input",        .match=1, .fn=cmd_set_session_input,      .help=NULL}
    , {.name="monochrome",   .match=1, .fn=cmd_set_session_monochrome, .help=NULL}
    , {.name="ncols",        .match=1, .fn=cmd_set_session_ncols,      .help=NULL}
    , {.name="winsz",        .match=1, .fn=cmd_set_session_winsz,      .help=NULL}
    , {0}
    };
Err cmd_set_session(CmdParams p[static 1]) { return run_cmd__(p, _cmd_set_session_); }

static SessionCmd _cmd_set_[] = 
    { {.name="curl",    .match=1, .fn=cmd_set_curl,    .help=CMD_SET_CURL}
    , {.name="session", .match=1, .fn=cmd_set_session, .help=NULL,.subcmds=_cmd_set_session_}
    , {0}
    };
#define CMD_SET_DOC "Sets an option.\n"
Err cmd_set(CmdParams p[static 1]) {
    return run_cmd__(p, _cmd_set_);
}

Err dbg_print_form(CmdParams p[static 1]) ;

Err shortcut_z(Session session[static 1], const char* rest);
#define CMD_SHORTCUT_Z \
    "zN print the following N lines"
Err cmd_shortcut_z(CmdParams p[static 1]) { return shortcut_z(p->s, p->ln); }

#define CMD_BOOKMARKS_DOC \
    "This command assumes the current document is a bookmarks doc. A bookmarks \n"\
    "doc is any one that has the structure of w3m bookmark.html (that is stored \n"\
    " in $HOME/.w3m by w3m.\n"\
    "In ahre we open the file at $HOME/.w3m/bookmark.html by \\go \\bookmark.\n\n"\
    "the bookamrks command list the bookmars file sections.\n\n"\
    "TODO: is this command useful at all?\n"
Err cmd_bookmarks(CmdParams p[static 1]);


#define CMD_HELP_DOC \
    "Ahre has char commands and word commands.\n" \
    "Word commands can be called with a substring, as long as there is no ambiguity.\n"\
    "Type SUB_COMMAND ? to get help for help in a sub command:\n\n"
Err cmd_help(CmdParams p[static 1]);

#define CMD_HELP_IX 14
static SessionCmd _session_cmd_[] = 
    { [0]={.name="bookmarks", .match=1, .fn=cmd_bookmarks, .help=CMD_BOOKMARKS_DOC}
    , [1]={.name="cookies",   .match=1, .fn=cmd_cookies,   .help=CMD_COOKIES_DOC}
    , [2]={.name="echo",      .match=1, .fn=cmd_echo,      .help=CMD_ECHO_DOC}
    , [3]={.name="go",        .match=1, .fn=cmd_go,        .help=CMD_GO_DOC}
    , [4]={.name="quit",      .match=1, .fn=cmd_quit,      .help=CMD_QUIT_DOC, .flags=CMD_NO_PARAMS}
    , [5]={.name="set",       .match=1, .fn=cmd_set,       .help=CMD_SET_DOC, .subcmds=_cmd_set_}
    , [6]={.name="|",  .fn=cmd_tabs,       .help=CMD_TABS_DOC, .flags=CMD_CHAR, .subcmds=_cmd_tabs_}
    , [7]={.name=".",  .fn=cmd_doc,        .help=CMD_DOC_DOC, .flags=CMD_CHAR, .subcmds=_cmd_doc_}
    , [8]={.name=":",  .fn=cmd_textbuf,    .help=NULL, .flags=CMD_CHAR, .subcmds=_cmd_textbuf_}
    , [9]={.name="<",  .fn=cmd_sourcebuf,  .help=NULL, .flags=CMD_CHAR, .subcmds=_cmd_textbuf_}
    , [10]={.name="&", .fn=dbg_print_form, .help=NULL, .flags=CMD_CHAR}
    , [11]={.name=ANCHOR_OPEN_STR,.fn=cmd_anchor, .help=CMD_ANCHOR_DOC, .flags=CMD_CHAR,
        .subcmds=_cmd_anchor_}
    , [12]={.name=INPUT_OPEN_STR, .fn=cmd_input,  .help=CMD_INPUT_DOC, .flags=CMD_CHAR,
        .subcmds=_cmd_input_}
    , [13]={.name=IMAGE_OPEN_STR, .fn=cmd_image,  .help=CMD_IMAGE_DOC, .flags=CMD_CHAR}
    , [CMD_HELP_IX]={.name="?",   .fn=cmd_help, .help=CMD_HELP_DOC, .flags=CMD_CHAR,
        .subcmds=_session_cmd_}
    , [15]={.name="z", .fn=cmd_shortcut_z, .help=CMD_SHORTCUT_Z, .flags=CMD_CHAR}
    , [16]={0}
    };

Err process_line(Session session[static 1], const char* line) {
    if (!line) { session_quit_set(session); return "no input received, exiting"; }
    line = cstr_skip_space(line);
    if (*line == '\\') line = cstr_skip_space(line + 1);
    if (!*line) { return Ok; }
    return run_cmd__(&(CmdParams){.s=session,.ln=line}, _session_cmd_);
}

Err cmd_help(CmdParams p[static 1]) { return run_cmd_help(p->s, &_session_cmd_[CMD_HELP_IX]); }

Err process_line_line_mode(Session* s, const char* line) {
    if (!s) return "error: no session :./";
    return process_line(s, line);
}

Err process_line_vi_mode(Session* s, const char* line) {
    if (!s) return "error: no session :./";
    try( process_line(s, line));
    return session_doc_draw(s);
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
