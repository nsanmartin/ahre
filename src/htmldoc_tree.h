#ifndef AHRE_HTMLDOC_TREE_H__
#define AHRE_HTMLDOC_TREE_H__

#include "src/htmldoc.h"

struct ArlOf(HtmlDocNode);
typedef struct ArlOf(HtmlDocNode) ArlOf(HtmlDocNode);

typedef struct HtmlDocNode HtmlDocNode;
typedef struct HtmlDocNode {
    HtmlDocNode* parent; 
    HtmlDoc doc;
    ArlOf(HtmlDocNode)* childs;
    size_t current_ix;
} HtmlDocNode;

typedef struct {
    HtmlDocNode head; /* owner */
    HtmlDocNode* current; /* view if null then tree is empty */
} HtmlDocTree;

void htmldoc_node_cleanup(HtmlDocNode n[static 1]);

#define T HtmlDocNode
// #define TClean htmldoc_node_cleanup
#include <arl.h>

/* Node */
/* getters */
static inline ArlOf(HtmlDocNode)* htmldoc_node_childs(HtmlDocNode dn[static 1]) {
    return dn->childs;
}

static inline size_t* htmldoc_node_current_ix(HtmlDocNode dn[static 1]) {
    return &dn->current_ix;
}


static inline HtmlDoc* htmldoc_node_current_doc(HtmlDocNode n[static 1]) {
    HtmlDocNode* cn = arlfn(HtmlDocNode, at)(n->childs, n->current_ix);
    if (!cn) return NULL;
    return &cn->doc;
}


/* ctor */
static inline Err htmldoc_node_init(HtmlDocNode n[static 1]) {
    n->childs = std_malloc(sizeof *n->childs);
    //TODO: check failure here
    if (!n->childs) return "error: htmldoc_doc mem failure";
    *n->childs = (ArlOf(HtmlDocNode)){0};
    return Ok;
}


/* Tree */
/* getters */
static inline HtmlDocNode** htmldoc_tree_current_node(HtmlDocTree t[static 1]) {
    return &t->current;
}

static inline HtmlDocNode* htmldoc_tree_head(HtmlDocTree t[static 1]) {
    return &t->head;
}

static inline HtmlDoc* htmldoc_tree_current_doc(HtmlDocTree t[static 1]) {
    return htmldoc_node_current_doc(*htmldoc_tree_current_node(t));
}

/* ctor */
static inline Err htmldoc_tree_init(HtmlDocTree t[static 1]) {
    *t = (HtmlDocTree){0};
    return htmldoc_node_init(htmldoc_tree_head(t));
}

/* dtor */
void htmldoc_tree_cleanup(HtmlDocTree t[static 1]);

/**/
static inline
Err htmldoc_tree_append_url(HtmlDocTree t[static 1], const char* url) {
    HtmlDocNode* cn = *htmldoc_tree_current_node(t);
    if (cn) {
        HtmlDocNode new_node = (HtmlDocNode){.parent=cn};
        try( htmldoc_init(&new_node.doc, url));
        /* move */
        HtmlDocNode* new_current = arlfn(HtmlDocNode, append)(
            htmldoc_node_childs(cn), &new_node
        );
        if (!new_current) {
            htmldoc_cleanup(&new_node.doc);
            return "error: arl append failure";
        }

        *htmldoc_tree_current_node(t) = new_current;
    } else {
        try( htmldoc_init(&htmldoc_tree_head(t)->doc, url));
        cn = *htmldoc_tree_current_node(t) = htmldoc_tree_head(t);
    }
    size_t len = htmldoc_node_childs(cn)->len;
    *htmldoc_node_current_ix(cn) = len;
    return Ok;
}
#endif
