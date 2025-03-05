#ifndef AHRE_SESSION_H__
#define AHRE_SESSION_H__

#include "src/session-conf.h"
#include "src/htmldoc.h"
#include "src/tabs.h"
#include "src/user-input.h"
#include "src/user-out.h"
#include "src/utils.h"

//typedef struct HtmlDoc HtmlDoc;
typedef struct Session Session;

typedef Err (*UserLineCallback)(Session* session, const char*);


typedef struct Session {
    UrlClient* url_client;
    TabList tablist;
    SessionConf conf;
} Session;

/* getters */
Err session_current_buf(Session session[static 1], TextBuf* out[static 1]);
Err session_current_doc(Session session[static 1], HtmlDoc* out[static 1]);

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
static inline UserInput* session_uin(Session s[static 1]) { return session_conf_uin(session_conf(s)); }
static inline size_t*
session_nrows(Session s[static 1]) { return session_conf_nrows(session_conf(s)); }
static inline size_t*
session_ncols(Session s[static 1]) { return session_conf_ncols(session_conf(s)); }

static inline TabList*
session_tablist(Session s[static 1]) { return &s->tablist; }

static inline WriteUserOutputCallback session_write_msg(Session s[static 1]) {
    return session_conf_write_msg(session_conf(s));
}
/* ctor */
Err session_init(Session s[static 1], SessionConf sconf[static 1]);

/* dtor */
static inline void session_cleanup(Session s[static 1]) {
    url_client_destroy(s->url_client);
    tablist_cleanup(session_tablist(s));
}

void session_destroy(Session* session);

/**/

static inline Err
session_open_url(Session s[static 1], const char* url, UrlClient url_client[static 1]) {
    return tablist_append_tree_from_url(
        session_tablist(s), url, url_client, session_conf(s)
    );
}

static inline Err session_follow_ahref(Session s[static 1], size_t linknum) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_ahref(current_tab , linknum, s->url_client, session_conf(s));
    
    return "error: where is the href if current tree is empty?";
}

static inline Err session_press_submit(Session s[static 1], size_t ix) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_submit(
            current_tab , ix, session_url_client(s), session_conf(s)
        );

    
    return "error: where is the input if current tree is empty?";
}

int edcmd_print(Session session[static 1]);

Err dbg_session_summary(Session session[static 1]);
Err cmd_set(Session session[static 1], const char* line);
#endif
