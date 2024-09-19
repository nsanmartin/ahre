#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include <ah/utils.h>
#include <ah/doc.h>
#include <ah/wrappers.h>

typedef struct Session Session;

typedef int (*AhUserLineCallback)(Session* session, const char*);


typedef struct Session {
    UrlClient* ahcurl;
    Doc* ahdoc;
    bool quit;
    int (*user_line_callback)(Session* session, const char*);
} Session;

TextBuf* AhCtxCurrentBuf(Session session[static 1]);
Doc* AhCtxCurrentDoc(Session session[static 1]);

Session* AhCtxCreate(char* url, AhUserLineCallback callback);
void AhCtxFree(Session* session) ;
int ah_ed_cmd_print(Session session[static 1]);

int AhCtxBufSummary(Session session[static 1]);
#endif
