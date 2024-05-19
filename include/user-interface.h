#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

typedef struct AhreCtx AhreCtx;

typedef struct AhreCtx {
    const char* url;
    int (*user_line_callback)(AhreCtx* ctx, const char*);
    bool quit;
} AhreCtx;

//typedef struct {
//    int (*callback)(const char*);
//} UserLineCallback;

int ahre_read_line_from_user(AhreCtx ctx[static 1]);
int ahre_process_line(AhreCtx ctx[static 1], const char* line);

#endif
