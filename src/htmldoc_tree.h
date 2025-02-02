#ifndef AHRE_HTMLDOC_TREE_H__
#define AHRE_HTMLDOC_TREE_H__

#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"
#include "src/wrapper-lexbor-curl.h"

struct ArlOf(TabNode);
typedef struct ArlOf(TabNode) ArlOf(TabNode);

typedef struct TabNode TabNode;
typedef struct TabNode {
    TabNode* parent; 
    HtmlDoc doc;
    ArlOf(TabNode)* childs;
    size_t current_ix; /* if ix == childs.len => current == doc */
} TabNode;

typedef struct {
    TabNode head; /* owner */
} HtmlDocTree;

void tab_node_cleanup(TabNode n[static 1]);

#define T TabNode
// #define TClean tab_node_cleanup
#include <arl.h>

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
    if (!arlfn(TabNode, append)(tab_node_childs(n), newnode))
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
    if (!n->childs) return "error: tab_node_init mem failure";
    *n->childs = (ArlOf(TabNode)){0};
    n->parent = parent;
    return Ok;
}

static inline Err
tab_node_init_from_curlu(
    TabNode n[static 1], TabNode* parent, CURLU* cu, UrlClient url_client[static 1]
) {
    try(_tab_node_init_base_(n, parent));
    try( htmldoc_init_fetch_browse_from_curlu(tab_node_doc(n), cu, url_client));
    n->current_ix = n->childs->len;
    return Ok;
}

static inline Err
tab_node_init(
    TabNode n[static 1], TabNode* parent, const char* url, UrlClient url_client[static 1]
) {
    try(_tab_node_init_base_(n, parent));
    Err e = htmldoc_init_fetch_browse(tab_node_doc(n), url, url_client);
    if (e) {
        tab_node_cleanup(n);
        return e;
    }
    n->current_ix = n->childs->len;
    return Ok;
}


/* Tree */
/* getters */
static inline TabNode* htmldoc_tree_head(HtmlDocTree t[static 1]) { return &t->head; }
static inline HtmlDoc* htmldoc_tree_head_doc(HtmlDocTree t[static 1]) {
    return &htmldoc_tree_head(t)->doc;
}
static inline Err htmldoc_tree_current_doc(HtmlDocTree t[static 1], HtmlDoc* out[static 1]) {
    try( tab_node_current_doc(htmldoc_tree_head(t), out));
    return Ok;

}

static inline Err htmldoc_tree_current_node(HtmlDocTree t[static 1], TabNode* out[static 1]) {
    return tab_node_current_node(htmldoc_tree_head(t), out);
}


/* ctor */
static inline Err
htmldoc_tree_init(HtmlDocTree t[static 1], const char* url, UrlClient url_client[static 1]) {
    *t = (HtmlDocTree){0};
    try( tab_node_init(htmldoc_tree_head(t), 0x0, url, url_client));
    return Ok;
}

/* dtor */
void htmldoc_tree_cleanup(HtmlDocTree t[static 1]);

/**/
static inline
Err htmldoc_tree_append_ahref(HtmlDocTree t[static 1], size_t linknum, UrlClient url_client[static 1])
{
    TabNode* n;
    try(  htmldoc_tree_current_node(t, &n));
    if (!n) return "error: current node not found";
    HtmlDoc* d = &n->doc;
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, linknum);
    if (!a) return "link number invalid";

    CURLU* curlu = url_cu(htmldoc_url(d));
    try( lexcurl_dup_curl_with_anchors_href(*a, &curlu));

    TabNode newnode;
    try( tab_node_init_from_curlu(&newnode, n, curlu, url_client));
    Err err;
    if ((err=tab_node_append_move_child(n, &newnode))) {
        curl_url_cleanup(curlu);
        return err;
    }
    return Ok;
}

static inline
Err htmldoc_tree_append_submit(HtmlDocTree t[static 1], size_t ix, UrlClient url_client[static 1])
{
    TabNode* n;
    try(  htmldoc_tree_current_node(t, &n));
    if (!n) return "error: current node not found";
    HtmlDoc* d = &n->doc;
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(d);
    CURLU* curlu = curl_url_dup(url_cu(htmldoc_url(d)));
    if (!curlu) return "error: memory failure (curl_url_dup)";
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, ix);
    if (!nodeptr) {
        curl_url_cleanup(curlu);
        return "link number invalid";
    }
    if (!_lexbor_attr_has_value(*nodeptr, "type", "submit")) {
        curl_url_cleanup(curlu);
        return "warn: not submit input";
    }

    lxb_dom_node_t* form = _find_parent_form(*nodeptr);
    if (form) {
        HttpMethod method;
        Err err;
        if ((err = mk_submit_url(url_client, form, &curlu, &method))) {
            curl_url_cleanup(curlu);
            return err;
        }

        TabNode newnode;
        try( tab_node_init_from_curlu(&newnode, n, curlu, url_client));
        if ((err=tab_node_append_move_child(n, &newnode))) {
            curl_url_cleanup(curlu);
            return err;
        }
    } else { puts("expected form, not found"); }
    return Ok;
}

static inline
Err htmldoc_tree_append_url(HtmlDocTree t[static 1], const char* url) {
    TabNode* cn;
    try( htmldoc_tree_current_node(t, &cn));
    if (!cn) { /* head is current node */
        cn = htmldoc_tree_head(t);
    }

    TabNode new_node = (TabNode){.parent=cn};
    try( htmldoc_init(&new_node.doc, url));
    /* move */
    TabNode* new_current = arlfn(TabNode, append)(
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

static inline 
Err dbg_tab_node_print(TabNode n[static 1], size_t ix, size_t h) {
    HtmlDoc* d = &n->doc;
    LxbNodePtr node = *htmldoc_title(d);
    if (h) {
        if (2*h > INT_MAX) return "error: well that's a large tree";
        printf("%*c", (int)(2*h), ' ');
    }
    printf("%ld : ", ix);
    try( dbg_print_title(node));

    TabNode* it = arlfn(TabNode, begin)(n->childs);
    //const TabNode* beg = it;
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) {
        try( dbg_tab_node_print(it, ix, h+1));
    }
    return Ok;
}
#endif
