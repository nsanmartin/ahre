#ifndef AHRE_SESSION_H__
#define AHRE_SESSION_H__

#include "src/session-conf.h"
#include "src/htmldoc.h"
#include "src/tabs.h"
#include "src/user-input.h"
#include "src/user-out.h"
#include "src/utils.h"


typedef struct Session Session;

typedef Err (*UserLineCallback)(Session* session, const char*);


typedef struct Session {
    UrlClient* url_client;
    TabList tablist;
    SessionConf conf;
    /* accidental data */
    Str msg; //TODO: add one msg buffer per htmldoc
} Session;


Err process_line(Session session[static 1], const char* line);

/* getters */
Err session_current_buf(Session session[static 1], TextBuf* out[static 1]);
Err session_current_doc(Session session[static 1], HtmlDoc* out[static 1]);

static inline Str* session_msg(Session s[static 1]) { return &s->msg; }

static inline UrlClient* session_url_client(Session session[static 1]) {
    return session->url_client;
}
static inline SessionConf* session_conf(Session s[static 1]) { return &s->conf; }
static inline bool session_quit(Session s[static 1]) { return session_conf_quit(session_conf(s)); }
static inline void session_quit_set(Session s[static 1]) { session_conf_quit_set(session_conf(s)); }
static inline bool
session_monochrome(Session s[static 1]) { return session_conf_monochrome(session_conf(s)); }
static inline void
session_monochrome_set(Session s[static 1], bool value) {
    session_conf_monochrome_set(session_conf(s), value);
}
static inline UserInterface* session_ui(Session s[static 1]) { return session_conf_ui(session_conf(s)); }
static inline UserInput* session_uinput(Session s[static 1]) { return session_conf_uinput(session_conf(s)); }
static inline UserOutput* session_uout(Session s[static 1]) { return session_conf_uout(session_conf(s)); }
static inline size_t*
session_nrows(Session s[static 1]) { return session_conf_nrows(session_conf(s)); }
static inline size_t*
session_ncols(Session s[static 1]) { return session_conf_ncols(session_conf(s)); }

static inline TabList*
session_tablist(Session s[static 1]) { return &s->tablist; }

static inline bool session_is_empty(Session s[static 1]) { return !session_tablist(s)->tabs.len; }
static inline SessionWriteFn
session_doc_log_fn(Session s[static 1], HtmlDoc d[static 1]) {
    (void)d;
    return (SessionWriteFn) {.write=session_uout(s)->write_msg, .ctx=s};
}


/* ctor */
Err session_init(Session s[static 1], SessionConf sconf[static 1]);

/* dtor */
static inline void session_cleanup(Session s[static 1]) {
    url_client_destroy(s->url_client);
    tablist_cleanup(session_tablist(s));
    str_clean(session_msg(s));
}

void session_destroy(Session* session);

/**/

Err tablist_append_tree_from_url(
    TabList f[static 1],
    const char* url,
    UrlClient url_client[static 1],
    Session s[static 1]
) ;
static inline Err
session_open_url(Session s[static 1], const char* url, UrlClient url_client[static 1]) {
    return tablist_append_tree_from_url(session_tablist(s), url, url_client, s);
}

Err tab_node_tree_append_ahref(
    TabNode t[static 1],
    size_t linknum,
    UrlClient url_client[static 1],
    Session s[static 1]
);
static inline Err session_follow_ahref(Session s[static 1], size_t linknum) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_ahref(current_tab , linknum, s->url_client, s);
    
    return "error: where is the href if current tree is empty?";
}

Err tab_node_tree_append_submit(
    TabNode t[static 1],
    size_t ix,
    UrlClient url_client[static 1],
    Session s[static 1]
);
static inline Err session_press_submit(Session s[static 1], size_t ix) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_submit(current_tab , ix, session_url_client(s), s);

    
    return "error: where is the input if current tree is empty?";
}

int edcmd_print(Session session[static 1]);

Err dbg_session_summary(Session session[static 1]);
Err cmd_set(Session session[static 1], const char* line);

static inline Err session_read_user_input(Session s[static 1], char* line[static 1]) {
    return session_uinput(s)->read(s, NULL, line);
}


static inline Err session_consume_line(Session s[static 1], char* user_input) {
    Err err = process_line(s, user_input);
    std_free(user_input);
    return err;
}

static inline Err session_write_msg(Session s[static 1], char* msg, size_t len) {
    UserOutput* uo = session_uout(s);
    return uo->write_msg(msg, len, s);
}

static inline Err session_write_std(Session s[static 1], char* msg, size_t len) {
    UserOutput* uo = session_uout(s);
    return uo->write_std(msg, len, s);
}

#define session_write_msg_lit__(Ses, LitStr) session_write_msg(Ses, LitStr, lit_len__(LitStr))
#define session_write_std_lit__(Ses, LitStr) session_write_std(Ses, LitStr, lit_len__(LitStr))

static inline Err session_write_unsigned_std(Session s[static 1], uintmax_t ui) {
    return ui_write_unsigned(session_conf_uout(session_conf(s))->write_std, ui, s);
}

static inline Err session_write_unsigned_msg(Session s[static 1], uintmax_t ui) {
    return ui_write_unsigned(session_conf_uout(session_conf(s))->write_msg, ui, s);
}


static inline Err session_show_error(Session s[static 1], Err err) {
    size_t len = strlen(err);
    return session_uout(s)->show_err(s, (char*)err, len);
}

static inline Err session_show_output(Session s[static 1]) {
    return session_uout(s)->show_session(s);
}


/* tablist ctor */

static inline Err
tablist_init(
    TabList f[static 1],
    const char* url,
    UrlClient url_client[static 1],
    Session s[static 1]
) {
    return tablist_append_tree_from_url(f, url, url_client, s);
}
#endif
