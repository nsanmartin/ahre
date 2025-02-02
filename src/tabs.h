#ifndef AHRE_TABS_H__
#define AHRE_TABS_H__
#include "src/htmldoc_tree.h"

#define T HtmlDocTree
// #define TClean htmldoc_tree_cleanup
#include <arl.h>

void arl_of_htmldoc_tree_clean(ArlOf(HtmlDocTree)* t);

typedef struct {
    ArlOf(HtmlDocTree) tabs;
    size_t current_tab;
} TabList;


/* getter */

static inline ArlOf(HtmlDocTree)* _tablist_tabs_(TabList f[static 1]) {
    return &f->tabs;
}

static inline size_t _tablist_tab_count_(TabList f[static 1]) {
    return _tablist_tabs_(f)->len;
}

static inline bool tablist_is_empty(TabList f[static 1]) {
    return _tablist_tab_count_(f) == 0;
}

static inline Err
tablist_current_tab(TabList f[static 1], HtmlDocTree* out[static 1]) {
    HtmlDocTree* current_tab = arlfn(HtmlDocTree, at)(&f->tabs, f->current_tab);
    if (!current_tab && !tablist_is_empty(f)) return "error: not current tree available";
    *out = current_tab;
    return Ok;
}

static inline Err tablist_current_doc(TabList f[static 1], HtmlDoc* out[static 1]) {
    HtmlDocTree* t;
    try( tablist_current_tab(f, &t));
    if (!t) { 
        *out = NULL;
        return Ok;
    }
    HtmlDoc* d;
    try( htmldoc_tree_current_doc(t, &d));
    *out = d;
    return Ok;
}

static inline Err
tablist_append_move_tree(TabList f[static 1], HtmlDocTree t[static 1]) {
    if (!arlfn(HtmlDocTree, append)(&f->tabs, t)) return "error: arl append failure";
    return Ok;
}

static inline Err tablist_append_tree_from_url(
    TabList f[static 1], const char* url, UrlClient url_client[static 1]
) {
    HtmlDocTree t = (HtmlDocTree){0};
    try( htmldoc_tree_init(&t, url, url_client));
    try( tablist_append_move_tree(f, &t));
    if (!f->tabs.len) return "error: expecting tabs in the tab list after appending a tab";
    f->current_tab = f->tabs.len - 1;
    return Ok;
}


/* ctor */

static inline Err tablist_init(
    TabList f[static 1], const char* url, UrlClient url_client[static 1]
) {
    return tablist_append_tree_from_url(f, url, url_client);
}

/* dtor */
static inline void tablist_cleanup(TabList f[static 1]) {
    arl_of_htmldoc_tree_clean(&f->tabs);
}
#endif
