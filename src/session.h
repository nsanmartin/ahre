#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include "src/utils.h"
#include "src/doc.h"
#include "src/wrappers.h"

typedef struct Session Session;

typedef Err (*UserLineCallback)(Session* session, const char*);


typedef struct Session {
    UrlClient* url_client;
    Doc* doc;
    bool quit;
    Err (*user_line_callback)(Session* session, const char*);
} Session;

TextBuf* session_current_buf(Session session[static 1]);
Doc* session_current_doc(Session session[static 1]);

Session* session_create(char* url, UserLineCallback callback);
void session_destroy(Session* session) ;
int edcmd_print(Session session[static 1]);

Err dbg_session_summary(Session session[static 1]);
#endif
