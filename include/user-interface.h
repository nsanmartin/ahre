#ifndef __USER_INTERFACE_AHRE_H__
#define __USER_INTERFACE_AHRE_H__

#include <stdbool.h>

#include <ahctx.h>
#include <ahutils.h>

int ah_read_line_from_user(AhCtx ctx[static 1]);
int ah_process_line(AhCtx ctx[static 1], char* line);

static inline char* skip_space(char* s) { for (; *s && isspace(*s); ++s); return s; }
static inline char* next_space(char* l) {
    while (*l && !isspace(*l)) { ++l; }
    return l;
}
static inline char* trim_space(char* l) {
    l = skip_space(l);
    char* r = next_space(l);
    *r = '\0';
    return l;
}

void print_html(lxb_html_document_t* document);
void lexbor_print_html_text(lxb_html_document_t* document);

int lexbor_href_write(
    lxb_html_document_t* document,
    lxb_dom_collection_t** hrefs,
    BufOf(char)* buf
);
#endif
