#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include <ah/utils.h>
#include <ah/doc.h>
#include <ah/wrappers.h>

typedef struct Session Session;

typedef int (*UserLineCallback)(Session* session, const char*);


typedef struct Session {
    UrlClient* ahcurl;
    Doc* ahdoc;
    bool quit;
    int (*user_line_callback)(Session* session, const char*);
} Session;

TextBuf* session_current_buf(Session session[static 1]);
Doc* session_current_doc(Session session[static 1]);

Session* session_create(char* url, UserLineCallback callback);
void session_destroy(Session* session) ;
int edcmd_print(Session session[static 1]);

int AhCtxBufSummary(Session session[static 1]);
#endif
