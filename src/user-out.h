#ifndef AHRE_USER_OUT_H__
#define AHRE_USER_OUT_H__

typedef enum {
    uiout_stdout
} UiOut;
static inline Err ui_out_print(UiOut, uiout, StrView s) {
    switch (uiout)
    case uiout_stdout:
        if (s.len != fwrite(s.s, 1, s.len, stdout)) return "error: fwrite failure";
        return Ok;
    default: return "error: invalid ui out";
}
#endif
