#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include "src/ranges.h"
#include "src/session.h"
#include "src/utils.h"
#include "src/cmd-ed.h"

Err read_line_from_user(Session session[static 1]);
Err process_line(Session session[static 1], const char* line);


void print_html(lxb_html_document_t* document);
Err lexbor_html_text_append(lxb_html_document_t* document, TextBuf* buf);


Err cmd_open_url(Session session[static 1], const char* url);

Err ui_get_win_size(size_t nrows[static 1], size_t ncols[static 1]) ;
#endif
