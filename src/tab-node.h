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

void tab_node_cleanup(TabNode n[_1_]);

#define T TabNode
// #define TClean tab_node_cleanup //TODO: uncomment
#include <arl.h>

TabNode* arl_of_tab_node_append(ArlOf(TabNode)* list, TabNode tn[_1_]);

/* Node */
/* getters */
static inline ArlOf(TabNode)* tab_node_childs(TabNode dn[_1_]) { return dn->childs; }
static inline size_t tab_node_child_count(TabNode dn[_1_]) { return dn->childs->len; }
static inline HtmlDoc* tab_node_doc(TabNode n[_1_]) { return &n->doc; }
static inline bool
tab_node_have_current_child(TabNode n[_1_]) {return n->current_ix != tab_node_child_count(n);}
static inline TabNode*
tab_node_current_child(TabNode n[_1_]) {return arlfn(TabNode, at)(n->childs, n->current_ix);}

static inline bool tab_node_is_current_in_tab(TabNode n[_1_]) {
    if (tab_node_have_current_child(n)) return false;
    if (!n->parent) return true;
    while (n && n->parent && n == tab_node_current_child(n->parent)) {
        n = n->parent;
    }
    return !n || n->parent == NULL;
}


static inline Err tab_node_current_node(TabNode n[_1_], TabNode* out[_1_]) {
    TabNode* prev = NULL;
    while (n) {
        prev = n;
        n = arlfn(TabNode, at)(n->childs, n->current_ix);
    }
    if (!prev) return "error: tab_node_current_node failure";
    *out = prev;
    return Ok;
}

static inline Err tab_node_current_doc(TabNode n[_1_], HtmlDoc* out[_1_] ) {
    TabNode* cn;
    try( tab_node_current_node(n, &cn));
    if (!cn) return "error: current node in NULL";
    *out = &(cn->doc);
    return Ok;
}

static inline Err
tab_node_append_move_child(TabNode n[_1_], TabNode newnode[_1_]) {
    if (!arl_of_tab_node_append(tab_node_childs(n), newnode))
        return "error: mem failure arl";
    n->current_ix = tab_node_child_count(n);
    if (!n->current_ix) return "error: no items after appending";
    --n->current_ix; /* the ix of last one */
    return Ok;
}

/* ctor */
static inline Err
_tab_node_init_base_(TabNode n[_1_], TabNode* parent) {
    *n = (TabNode){0};
    n->childs = std_malloc(sizeof *n->childs);
    if (!n->childs) return "error: tab_node_init_from_cstr mem failure";
    *n->childs = (ArlOf(TabNode)){0};
    n->parent = parent;
    return Ok;
}


/**/

Err session_tab_node_print(
    Session* s,
    TabNode n[_1_],
    size_t ix,
    ArlOf(size_t) stack[_1_],
    TabNode* current_node
);


static inline Err
tab_node_get_child_index_of(TabNode n[_1_], TabNode child[_1_], size_t out[_1_]) {
    TabNode* beg = arlfn(TabNode, begin)(tab_node_childs(n));
    TabNode* end = arlfn(TabNode, end)(tab_node_childs(n));
    if (child < beg || end <= child) return "error: pointer not in range (tabnode not a child of)";
    *out = child - beg;
    return Ok;
}

static inline Err tab_node_set_as_current(TabNode n[_1_]) {
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
tab_node_find_node(TabNode tn[_1_], const char* line, TabNode* out[_1_]) {
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
