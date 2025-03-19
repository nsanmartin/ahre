#ifndef AHRE_USER_OUT_VI_MODE_H__
#define AHRE_USER_OUT_VI_MODE_H__

typedef struct Session Session;
typedef struct UserOutput UserOutput;

Err _vi_show_session_(Session* s);
Err _vi_write_msg_(const char* mem, size_t len, Session* s);
Err _vi_flush_msg_(Session* s);
Err _vi_show_err_(Session* s, char* err, size_t len);
Err _vi_flush_std_(Session* s);
Err _vi_write_std_(const char* mem, size_t len, Session* s);

typedef struct {
    Str msg;
    Str std;
    void (*dtor)(UserOutput *);
} ViOutCtx ;

/* getter */

void _vi_cleanup_(UserOutput* uo);
#endif
