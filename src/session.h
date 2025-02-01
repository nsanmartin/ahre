#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include "src/utils.h"
#include "src/htmldoc.h"
#include "src/htmldoc_forest.h"

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
    HtmlDoc* htmldoc;
    HtmlDocForest htmldoc_forest;
    bool quit;
    SessionConf conf;
    //TODO: do not use a callback here
    Err (*user_line_callback)(Session* session, const char*);
} Session;

/* getters */
TextBuf* session_current_buf(Session session[static 1]);
HtmlDoc* session_current_doc(Session session[static 1]);
static inline UrlClient* session_url_client(Session session[static 1]) {
    return session->url_client;
}
static inline SessionConf* session_conf(Session s[static 1]) { return &s->conf; }
static inline size_t* session_conf_z_shorcut_len(Session s[static 1]) {
    return &session_conf(s)->z_shorcut_len;
}

static inline HtmlDocForest*
session_htmldoc_forest(Session s[static 1]) { return &s->htmldoc_forest; }

/* ctor */
Session* session_create(char* url, UserLineCallback callback);

/* dtor */
void session_destroy(Session* session) ;

/**/

int edcmd_print(Session session[static 1]);

Err dbg_session_summary(Session session[static 1]);
Err cmd_setopt(Session session[static 1], const char* line);
#endif
