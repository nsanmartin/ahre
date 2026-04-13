#ifndef AHRE_BOOKMARK_H__
#define AHRE_BOOKMARK_H__

#include <errno.h>

#include "dom.h"
#include "utils.h"
#include "htmldoc.h"
#include "error.h"
#include "fetch-history.h"
#include "dom.h"
#include "session.h"


#define AHRE_BOOKMARK_HEAD "<html><head><title>Bookmarks</title></head>\n<body>\n"
#define AHRE_BOOKMARK_TAIL "</body>\n</html>\n"
#define EMPTY_BOOKMARK AHRE_BOOKMARK_HEAD "<h1>Bookmarks</h1>\n" AHRE_BOOKMARK_TAIL

Err cmd_bookmarks(CmdParams p[_1_]);
Err bookmark_sections_body(HtmlDoc bookmark[_1_], DomNode out[_1_]);
Err bookmark_sections(DomNode body, ArlOf(Str)* out);
Err bookmark_section_insert(Dom dom, DomNode body, const char* q, DomElem bm_entry);
Err bookmark_section_get(DomNode body, const char* q, DomNode out[_1_], bool match_prefix);
Err bookmark_section_ul_get(DomNode body, const char* q, DomNode out[_1_], bool match_prefix);
Err bookmark_mk_anchor (Dom dom, char* href, Str text[_1_], DomElem out[_1_]);
Err bookmark_mk_entry(Dom document, char* href, Str text[_1_], DomElem out[_1_]);
Err bookmarks_save_to_disc(HtmlDoc bm[_1_], StrView bm_path);
Err get_bookmarks_doc(UrlClient url_client[_1_], StrView bm_path, CmdOut cmd_out[_1_], HtmlDoc htmldoc_out[_1_]);
Err bookmark_add_to_section(Session s[_1_], const char* line, UrlClient url_client[_1_], CmdOut out[_1_]);
#endif
