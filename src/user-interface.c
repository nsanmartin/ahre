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
#define CMD_NO_PARAMS 0x1
#define CMD_CHAR 0x2
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
bool _char_cmd_match_(SessionCmd cmd[static 1], const char* line) {
    return cmd->flags & CMD_CHAR && cmd->name[0] == line[0];
}

bool _no_params_match_(SessionCmd cmd[static 1], const char* rest) {
    return cmd->flags & CMD_NO_PARAMS && *cstr_skip_space(rest);
}

Err
run_cmd__(Session s[static 1], const char* line, SessionCmd cmdlist[], SessionCmdFn default_cmd) {
    const char* rest = NULL;
    line = cstr_skip_space(line);
    for (SessionCmd* cmd = cmdlist; cmd->name ; ++cmd) {
        if (_char_cmd_match_(cmd, line)) {
            return cmd->fn(s, cstr_skip_space(line+1));
        } else if ((rest = csubstr_match(line, cmd->name, cmd->match))) {
            if (_no_params_match_(cmd, rest)) return  "unexpected param";
            else return cmd->fn(s, rest);
        }
    }
    return default_cmd(s, rest);
}

Err cmd_tabs(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_tabs_[] =
        { {.name="-", .fn=cmd_tabs_back, .help=NULL, .flags=CMD_CHAR}
        , {.name="?", .fn=cmd_tabs_info, .help=NULL, .flags=CMD_CHAR}
        , {0}
    };
    return run_cmd__(session, line, _session_cmd_tabs_, cmd_tabs_goto);
}


Err cmd_doc(Session session[static 1], const char* line) {
    static SessionCmd _session_cmd_doc_[] =
        { {.name="?", .fn=cmd_doc_info,           .help=NULL, .flags=CMD_CHAR}
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
