#ifndef AHRE_SESSION_H__
#define AHRE_SESSION_H__

#include "session-conf.h"
#include "tabs.h"
#include "user-input.h"
#include "user-out.h"
#include "utils.h"
#include "writer.h"
#include "reditline.h"
#include "fetch-history.h"

typedef struct Session {
    UrlClient          url_client;
    TabList            tablist;
    SessionConf        conf;

    Msg                      msg; //TODO: add one msg buffer per htmldoc?
    ArlOf(const_cstr)        input_history;
    Str                      dt_now_str;
    ArlOf(FetchHistoryEntry) fetch_history;
} Session;


Err process_line(Session session[_1_], const char* line);

/* getters */
Err session_current_buf(Session session[_1_], TextBuf* out[_1_]);
Err session_current_doc(Session session[_1_], HtmlDoc* out[_1_]);
Err session_current_src(Session session[_1_], TextBuf* out[_1_]);

static inline Str* session_dt_now_str(Session s[_1_]) { return &s->dt_now_str; }
static inline Msg* session_msg(Session s[_1_]) { return &s->msg; }

static inline UrlClient* session_url_client(Session session[_1_]) {
    return &session->url_client;
}
static inline SessionConf* session_conf(Session s[_1_]) { return &s->conf; }
static inline bool session_quit(Session s[_1_]) { return session_conf_quit(session_conf(s)); }
static inline void session_quit_set(Session s[_1_]) { session_conf_quit_set(session_conf(s)); }
static inline bool
session_monochrome(Session s[_1_]) { return session_conf_monochrome(session_conf(s)); }
static inline void
session_monochrome_set(Session s[_1_], bool value) {
    session_conf_monochrome_set(session_conf(s), value);
}
static inline void
session_js_set(Session s[_1_], bool value) { session_conf_js_set(session_conf(s), value); }
static inline UserInterface*
session_ui(Session s[_1_]) { return session_conf_ui(session_conf(s)); }
static inline UserInput*
session_uinput(Session s[_1_]) { return session_conf_uinput(session_conf(s)); }
static inline UserOutput*
session_uout(Session s[_1_]) { return session_conf_uout(session_conf(s)); }
static inline size_t*
session_nrows(Session s[_1_]) { return session_conf_nrows(session_conf(s)); }
static inline size_t*
session_ncols(Session s[_1_]) { return session_conf_ncols(session_conf(s)); }

static inline TabList*
session_tablist(Session s[_1_]) { return &s->tablist; }

static inline bool session_is_empty(Session s[_1_]) { return !session_tablist(s)->tabs.len; }
static inline SessionWriteFn
session_doc_msg_fn(Session s[_1_], HtmlDoc d[_1_]) {
    (void)d;
    return (SessionWriteFn) {.write=session_uout(s)->write_msg, .ctx=s};
}
static inline bool session_js(Session s[_1_]) { return session_conf_js(session_conf(s)); }
static inline ArlOf(const_cstr)* session_input_history(Session s[_1_]) { return &s->input_history; }
static inline ArlOf(FetchHistoryEntry)* session_fetch_history(Session s[_1_]) {
    return &s->fetch_history;
}

static inline Str* session_confdirname(Session s[_1_]) {
    return &session_conf(s)->confdirname;
}


static inline Str* session_bookmarks_fname(Session s[_1_]) {
    return &session_conf(s)->bookmarks_fname;
}

static inline Err session_bookmarks_fname_append(Session s[_1_], Str out[_1_]) {
    return str_append(
        out, session_conf(s)->bookmarks_fname.items, session_conf(s)->bookmarks_fname.len
    );
}

static inline Err session_fetch_history_fname_append(Session s[_1_], Str out[_1_]) {
    return str_append(
        out, session_conf(s)->fetch_history_fname.items, session_conf(s)->fetch_history_fname.len
    );
}

static inline Err session_input_history_fname_append(Session s[_1_], Str out[_1_]) {
    return str_append(
        out, session_conf(s)->input_history_fname.items, session_conf(s)->input_history_fname.len
    );
}

static inline StrView session_cookies_fname(Session s[_1_]) {
    return strview__(&s->conf.cookies_fname);
}

/* ctor */
Err session_init(Session s[_1_], SessionConf sconf[_1_]);

/* dtor */
static inline void session_cleanup(Session s[_1_]) {
    url_client_cleanup(session_url_client(s));
    tablist_cleanup(session_tablist(s));
    msg_clean(session_msg(s));
    reditline_history_cleanup(session_input_history(s));
    str_clean(session_dt_now_str(s));
    arlfn(FetchHistoryEntry,clean)(session_fetch_history(s));
    session_conf_cleanup(session_conf(s));
}

void session_destroy(Session* session);

/**/

Err session_close(Session s[_1_]);


Err tablist_append_tree_from_url(
    TabList   f[_1_],
    Request   r[_1_],
    UrlClient url_client[_1_],
    Session   s[_1_]
);


static inline Err session_fetch_request(
    Session     s[_1_],
    Request     r[_1_],
    UrlClient   url_client[_1_]
) { return tablist_append_tree_from_url(session_tablist(s), r, url_client, s); }

Err tab_node_tree_append_ahref(
    TabNode t[_1_],
    size_t linknum,
    UrlClient url_client[_1_],
    Session s[_1_]
);
static inline Err session_follow_ahref(Session s[_1_], size_t linknum) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_ahref(current_tab , linknum, session_url_client(s), s);
    
    return "error: where is the href if current tree is empty?";
}

Err tab_node_tree_append_submit(
    TabNode t[_1_],
    size_t ix,
    UrlClient url_client[_1_],
    Session s[_1_]
);
static inline Err session_press_submit(Session s[_1_], size_t ix) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_submit(current_tab , ix, session_url_client(s), s);

    
    return "error: where is the input if current tree is empty?";
}

int edcmd_print(Session session[_1_]);

Err dbg_session_summary(Session session[_1_]);

static inline Err session_read_user_input(Session s[_1_], UserLine ul[_1_]) {
    if (!user_line_empty(ul)) return Ok;
    char* line = NULL;
    try(session_uinput(s)->read(s, NULL, &line));
    return user_line_init_take_ownership(ul, line);
}


static inline Err session_consume_line(Session s[_1_], UserLine userln[_1_]) {
    const char* cmd = *user_line_remaining(userln);
    char* rest = strchr(cmd, ';');
    if (rest) {
        *user_line_remaining(userln) = rest + 1;
        rest[0] = '\0';
    } 

    Err err = session_ui(s)->process_line(s, cmd);
    if (!rest) user_line_cleanup(userln);
    return err;
}


static inline Err session_write_msg(Session s[_1_], char* msg, size_t len) {
    UserOutput* uo = session_uout(s);
    return uo->write_msg(msg, len, s);
}

#define session_write_msg_lit__(Ses, LitStr) session_write_msg(Ses, LitStr, lit_len__(LitStr))

static inline Err session_write_msg_ln(Session s[_1_], char* msg, size_t len) {
    UserOutput* uo = session_uout(s);
    try(uo->write_msg(msg, len, s));
    return session_write_msg_lit__(s, "\n");
}


static inline Err session_write_unsigned_std(Session s[_1_], uintmax_t ui) {
    return ui_write_unsigned(session_conf_uout(session_conf(s))->write_std, ui, s);
}


/* Writer To session std (ie screen) */
static inline Err session_write_std(Session s[_1_], char* msg, size_t len) {
    UserOutput* uo = session_uout(s);
    return uo->write_std(msg, len, s);
}

#define session_write_std_lit__(Ses, LitStr) session_write_std(Ses, LitStr, lit_len__(LitStr))

static inline Err session_write_std_ln(Session s[_1_], char* msg, size_t len) {
    UserOutput* uo = session_uout(s);
    try(uo->write_std(msg, len, s));
    return session_write_std_lit__(s, "\n");
}

static inline Err _writer_write_to_session_std_(Writer* self, const char* mem, size_t len) {
    UserOutput* uo = session_uout(self->out);
    return uo->write_std(mem, len, self->out);
}

static inline Err session_std_writer_init(Writer w[_1_], Session s[_1_]) {
    // if (!s) return "error: expecting a sessions, received NULL";
    *w = (Writer){ .self=w, .write=_writer_write_to_session_std_, .out=s };
    return Ok;
}

// Writer To session std (screen)

static inline Err session_write_unsigned_msg(Session s[_1_], uintmax_t ui) {
    return ui_write_unsigned(session_conf_uout(session_conf(s))->write_msg, ui, s);
}


static inline Err session_show_error(Session s[_1_], Err err) {
    size_t len = strlen(err);
    return session_uout(s)->show_err(s, (char*)err, len);
}

static inline Err session_show_output(Session s[_1_]) {
    return session_uout(s)->show_session(s);
}


/* Writer To session msg */
static inline Err writer_write_to_session_msg(Writer* self, const char* mem, size_t len) {
    UserOutput* uo = session_uout(self->out);
    return uo->write_msg(mem, len, self->out);
}

static inline Err session_msg_writer_init(Writer w[_1_], Session s[_1_]) {
    *w = (Writer){ .self=w, .write=writer_write_to_session_msg, .out=s };
    return Ok;
}
// Writer To session msg


Err
session_write_std_range_mod(Session s[_1_], TextBuf textbuf[_1_], Range range[_1_]);
Err session_write_input_history(Session s[_1_]);
#endif
