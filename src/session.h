#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include "src/utils.h"
#include "src/htmldoc.h"

typedef struct Session Session;

typedef Err (*UserLineCallback)(Session* session, const char*);


typedef struct Session {
    UrlClient* url_client;
    HtmlDoc* htmldoc;
    bool quit;
    Err (*user_line_callback)(Session* session, const char*);
} Session;

/* getters */
TextBuf* session_current_buf(Session session[static 1]);
HtmlDoc* session_current_doc(Session session[static 1]);
static inline UrlClient* session_url_client(Session session[static 1]) {
    return session->url_client;
}

/* ctor */
Session* session_create(char* url, UserLineCallback callback);

/* dtor */
void session_destroy(Session* session) ;

/**/

int edcmd_print(Session session[static 1]);

Err dbg_session_summary(Session session[static 1]);
#endif
