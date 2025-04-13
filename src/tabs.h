#ifndef AHRE_TABS_H__
#define AHRE_TABS_H__
#include "tab-node.h"

///#define T TabNode
///// #define TClean tab_node_cleanup
///#include <arl.h>

void arl_of_tab_node_clean(ArlOf(TabNode)* t);

typedef struct {
    ArlOf(TabNode) tabs;
    size_t current_tab;
} TabList;


/* getter */

static inline ArlOf(TabNode)* _tablist_tabs_(TabList f[static 1]) {
    return &f->tabs;
}

static inline size_t _tablist_tab_count_(TabList f[static 1]) {
    return _tablist_tabs_(f)->len;
}

static inline bool tablist_is_empty(TabList f[static 1]) {
    return _tablist_tab_count_(f) == 0;
}

static inline size_t* _tablist_current_tab_ix_(TabList f[static 1]) { return &f->current_tab; }
static inline Err
tablist_current_tab(TabList f[static 1], TabNode* out[static 1]) {
    TabNode* current_tab = arlfn(TabNode, at)(&f->tabs, f->current_tab);
    if (!current_tab) return "no tabs in session";
    if (tablist_is_empty(f)) return "error: current tab has no nodes";
    *out = current_tab;
    return Ok;
}

static inline Err
tablist_current_node(TabList f[static 1], TabNode* out[static 1]) {
    TabNode* root;
    try( tablist_current_tab(f, &root));
    if (!root) { 
        *out = NULL;
        return Ok;
    }

    return tab_node_current_node(root, out);
}

static inline Err tablist_current_doc(TabList f[static 1], HtmlDoc* out[static 1]) {
    TabNode* t;
    try( tablist_current_tab(f, &t));
    if (!t) { 
        *out = NULL;
        return Ok;
    }
    HtmlDoc* d;
    try( tab_node_current_doc(t, &d));
    *out = d;
    return Ok;
}

static inline Err
tablist_append_move_tree(TabList f[static 1], TabNode t[static 1]) {
    if (!arl_of_tab_node_append(&f->tabs, t))  return "error: arl append failure";
    return Ok;
}



/* dtor */
static inline void tablist_cleanup(TabList f[static 1]) {
    arl_of_tab_node_clean(&f->tabs);
}

Err tablist_info(Session* s,TabList f[static 1]);

static inline Err tablist_back(TabList tl[static 1]) {
    TabNode* cn;
    try( tablist_current_node(tl, &cn));
    if (!cn) return "can't go back with no current node";
    if (!cn->parent) return "can't go back if tab's root";
    tab_node_set_as_current(cn->parent);
    return Ok;
}

static inline Err tablist_move_to_node(TabList tl[static 1], const char* line) {
    size_t ix;
    try( parse_size_t_or_throw(&line, &ix, 10));
    TabNode* tn = arlfn(TabNode,at)(_tablist_tabs_(tl), ix);
    if (!tn) return "invalid tab";

    line = cstr_skip_space(line);
    if (!*line) {
        tl->current_tab = ix;
        return Ok;
    }
    if (*line != '.') return "invalid tab path";

    TabNode* search;
    try( tab_node_find_node(tn, line + 1, &search));
    try( tab_node_set_as_current(search));
    *_tablist_current_tab_ix_(tl) = ix;
    return Ok;

}
#endif
