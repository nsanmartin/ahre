#ifndef AHRE_USER_OUT_LINE_MODE_H__
#define AHRE_USER_OUT_LINE_MODE_H__

#include "error.h"

typedef struct Session Session;

Err _line_show_session_(Session* s);
Err _line_show_err_(Session* s, char* err, size_t len);

#endif
