#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include <ahdoc.h>

int ah_read_line_from_user(AhCtx ctx[static 1]);
int ah_process_line(AhCtx ctx[static 1], char* line);

static inline char* skip_space(char* s) { for (; *s && isspace(*s); ++s); return s; }

#endif
