#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include <ahctx.h>
#include <ahutils.h>

int ah_read_line_from_user(AhCtx ctx[static 1]);
int ah_process_line(AhCtx ctx[static 1], const char* line);


void print_html(lxb_html_document_t* document);
int
lexbor_html_text_append(lxb_html_document_t* document, AeBuf* buf);

int lexbor_href_write(
    lxb_html_document_t* document,
    lxb_dom_collection_t** hrefs,
    AeBuf* buf
);
#endif
