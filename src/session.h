#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include "src/utils.h"
#include "src/htmldoc.h"
#include "src/tabs.h"

typedef struct Session Session;

typedef Err (*UserLineCallback)(Session* session, const char*);

typedef struct {
    bool color;
    size_t maxcols;
    size_t z_shorcut_len;
} SessionConf ;

#define mkSessionConf (SessionConf){.color=true,.maxcols=90,.z_shorcut_len=42}

typedef struct Session {
    UrlClient* url_client;
    TabList tablist;
    bool quit;
    SessionConf conf;
    //TODO: do not use a callback here
    Err (*user_line_callback)(Session* session, const char*);
} Session;

/* getters */
Err session_current_buf(Session session[static 1], TextBuf* out[static 1]);
Err session_current_doc(Session session[static 1], HtmlDoc* out[static 1]);

static inline UrlClient* session_url_client(Session session[static 1]) {
    return session->url_client;
}
static inline SessionConf* session_conf(Session s[static 1]) { return &s->conf; }
static inline size_t* session_conf_z_shorcut_len(Session s[static 1]) {
    return &session_conf(s)->z_shorcut_len;
}

static inline TabList*
session_tablist(Session s[static 1]) { return &s->tablist; }

/* ctor */
Err session_init(Session s[static 1], char* url, UserLineCallback callback);

/* dtor */
static inline void session_cleanup(Session s[static 1]) {
    url_client_destroy(s->url_client);
    tablist_cleanup(session_tablist(s));
}

void session_destroy(Session* session);

/**/

static inline Err
session_open_url(Session s[static 1], const char* url, UrlClient url_client[static 1]) {
    return tablist_append_tree_from_url(session_tablist(s), url, url_client);
}

static inline Err session_follow_ahref(Session s[static 1], size_t linknum) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_ahref(current_tab , linknum, s->url_client);
    
    return "error: where is the href if current tree is empty?";
}

static inline Err session_press_submit(Session s[static 1], size_t ix) {
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab)
        return tab_node_tree_append_submit(current_tab , ix, session_url_client(s));

    
    return "error: where is the input if current tree is empty?";
}

int edcmd_print(Session session[static 1]);

Err dbg_session_summary(Session session[static 1]);
Err cmd_setopt(Session session[static 1], const char* line);
#endif
