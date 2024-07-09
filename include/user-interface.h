#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include <ahdoc.h>
#include <ahutils.h>

int ah_read_line_from_user(AhCtx ctx[static 1]);
int ah_process_line(AhCtx ctx[static 1], char* line);

static inline char* skip_space(char* s) { for (; *s && isspace(*s); ++s); return s; }

void print_html(lxb_html_document_t* document);
void lexbor_print_html_text(lxb_html_document_t* document);
#endif
