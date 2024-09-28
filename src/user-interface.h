#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include "src/ranges.h"
#include "src/session.h"
#include "src/utils.h"

Err read_line_from_user(Session session[static 1]);
Err process_line(Session session[static 1], const char* line);


void print_html(lxb_html_document_t* document);
int
lexbor_html_text_append(lxb_html_document_t* document, TextBuf* buf);

int lexbor_href_write(
    lxb_html_document_t* document,
    lxb_dom_collection_t** hrefs,
    TextBuf* buf
);

const char* substr_match(const char* s, const char* cmd, size_t len);
Err textbuf_eval_cmd(TextBuf textbuf[static 1], const char* line, Range range[static 1]);
#endif
