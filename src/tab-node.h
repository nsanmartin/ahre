#ifndef AHRE_TAB_NODE_H__
#define AHRE_TAB_NODE_H__

#include "htmldoc.h"
#include "wrapper-lexbor.h"
#include "wrapper-lexbor-curl.h"

struct ArlOf(TabNode);
typedef struct ArlOf(TabNode) ArlOf(TabNode);

typedef struct TabNode TabNode;
typedef struct TabNode {
    TabNode* parent; 
    HtmlDoc doc;
    ArlOf(TabNode)* childs;
    size_t current_ix; /* if ix == childs.len => current == doc */
} TabNode;

void tab_node_cleanup(TabNode n[static 1]);

#define T TabNode
// #define TClean tab_node_cleanup //TODO: uncomment
#include <arl.h>

TabNode* arl_of_tab_node_append(ArlOf(TabNode)* list, TabNode tn[static 1]);

/* Node */
/* getters */
static inline ArlOf(TabNode)* tab_node_childs(TabNode dn[static 1]) {
    return dn->childs;
}

static inline size_t tab_node_child_count(TabNode dn[static 1]) {
    return dn->childs->len;
}

static inline HtmlDoc* tab_node_doc(TabNode n[static 1]) {
    return &n->doc;
}

static inline bool tab_node_have_current_child(TabNode n[static 1]) {
    return n->current_ix != tab_node_child_count(n);
}

static inline TabNode* tab_node_current_child(TabNode n[static 1]) {
     return arlfn(TabNode, at)(n->childs, n->current_ix);
}

static inline bool tab_node_is_current_in_tab(TabNode n[static 1]) {
    if (tab_node_have_current_child(n)) return false;
    if (!n->parent) return true;
    while (n && n->parent && n == tab_node_current_child(n->parent)) {
        n = n->parent;
    }
    return !n || n->parent == NULL;
}


static inline Err tab_node_current_node(TabNode n[static 1], TabNode* out[static 1]) {
    TabNode* prev = NULL;
    while (n) {
        prev = n;
        n = arlfn(TabNode, at)(n->childs, n->current_ix);
    }
    if (!prev) return "error: tab_node_current_node failure";
    *out = prev;
    return Ok;
}

static inline Err tab_node_current_doc(TabNode n[static 1], HtmlDoc* out[static 1] ) {
    TabNode* cn;
    try( tab_node_current_node(n, &cn));
    if (!cn) return "error: current node in NULL";
    *out = &(cn->doc);
    return Ok;
}

static inline Err
tab_node_append_move_child(TabNode n[static 1], TabNode newnode[static 1]) {
    if (!arl_of_tab_node_append(tab_node_childs(n), newnode))
        return "error: mem failure arl";
    n->current_ix = tab_node_child_count(n);
    if (!n->current_ix) return "error: no items after appending";
    --n->current_ix; /* the ix of last one */
    return Ok;
}

/* ctor */
static inline Err
_tab_node_init_base_(TabNode n[static 1], TabNode* parent) {
    *n = (TabNode){0};
    n->childs = std_malloc(sizeof *n->childs);
    if (!n->childs) return "error: tab_node_init_from_cstr mem failure";
    *n->childs = (ArlOf(TabNode)){0};
    n->parent = parent;
    return Ok;
}


/**/

static inline
Err tab_node_tree_append_url(TabNode t[static 1], const char* url) {
    TabNode* cn;
    try( tab_node_current_node(t, &cn));
    if (!cn) { /* head is current node */
        cn = t;
    }

    TabNode new_node = (TabNode){.parent=cn};
    try( htmldoc_init_from_cstr(&new_node.doc, url, http_get));
    /* move */
    TabNode* new_current = arl_of_tab_node_append(
        tab_node_childs(cn), &new_node
    );
    if (!new_current) {
        htmldoc_cleanup(&new_node.doc);
        return "error: arl append failure";
    }

    size_t len = tab_node_childs(cn)->len;
    if (!len) return "error: no childs after append";
    cn->current_ix = len - 1;
    return Ok;
}



Err session_tab_node_print(
    Session* s,
    TabNode n[static 1],
    size_t ix,
    ArlOf(size_t) stack[static 1],
    TabNode* current_node
);


static inline Err
tab_node_get_child_index_of(TabNode n[static 1], TabNode child[static 1], size_t out[static 1]) {
    TabNode* beg = arlfn(TabNode, begin)(tab_node_childs(n));
    TabNode* end = arlfn(TabNode, end)(tab_node_childs(n));
    if (child < beg || end <= child) return "error: pointer not in range (tabnode not a child of)";
    *out = child - beg;
    return Ok;
}

static inline Err tab_node_set_as_current(TabNode n[static 1]) {
    n->current_ix = tab_node_child_count(n);
    TabNode* it = n->parent;
    while(it) {
        size_t ix;
        try( tab_node_get_child_index_of(it, n, &ix));
        it->current_ix = ix;
        n = it;
        it = n->parent;
    }
    return Ok;
}

static inline Err
tab_node_find_node(TabNode tn[static 1], const char* line, TabNode* out[static 1]) {
    line = cstr_skip_space(line);
    if (!*line) {
        *out = tn;
        return Ok;
    }
    size_t ix;
    try( parse_size_t_or_throw(&line, &ix, 10));
    tn = arlfn(TabNode,at)(tab_node_childs(tn), ix);
    if (!tn) return "invalid tab child";
    line = cstr_skip_space(line);
    if (!*line || *line != '.') return "invalid tab path (full path must en with dot)";
    return tab_node_find_node(tn, line + 1, out);
}

#endif
