#ifndef AHRE_HTMLDOC_TREE_H__
#define AHRE_HTMLDOC_TREE_H__

#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"
#include "src/wrapper-lexbor-curl.h"

struct ArlOf(HtmlDocNode);
typedef struct ArlOf(HtmlDocNode) ArlOf(HtmlDocNode);

typedef struct HtmlDocNode HtmlDocNode;
typedef struct HtmlDocNode {
    HtmlDocNode* parent; 
    HtmlDoc doc;
    ArlOf(HtmlDocNode)* childs;
    size_t current_ix; /* if ix == childs.len => current == doc */
} HtmlDocNode;

typedef struct {
    HtmlDocNode head; /* owner */
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

static inline size_t htmldoc_node_child_count(HtmlDocNode dn[static 1]) {
    return dn->childs->len;
}

static inline HtmlDoc* htmldoc_node_doc(HtmlDocNode n[static 1]) {
    return &n->doc;
}


static inline Err htmldoc_node_current_node(HtmlDocNode n[static 1], HtmlDocNode* out[static 1]) {
    HtmlDocNode* prev = NULL;
    while (n) {
        prev = n;
        n = arlfn(HtmlDocNode, at)(n->childs, n->current_ix);
    }
    if (!prev) return "error: htmldoc_node_current_doc failure";
    *out = prev;
    return Ok;
}

static inline Err  htmldoc_node_current_doc(HtmlDocNode n[static 1], HtmlDoc* out[static 1] ) {
    HtmlDocNode* cn;
    try( htmldoc_node_current_node(n, &cn));
    if (!cn) return "error: current node in NULL";
    *out = &(cn->doc);
    return Ok;
}

static inline Err
htmldoc_node_append_move_child(HtmlDocNode n[static 1], HtmlDocNode newnode[static 1]) {
    if (!arlfn(HtmlDocNode, append)(htmldoc_node_childs(n), newnode))
        return "error: mem failure arl";
    n->current_ix = htmldoc_node_child_count(n);
    if (!n->current_ix) return "error: no items after appending";
    --n->current_ix; /* the ix of last one */
    return Ok;
}

/* ctor */
static inline Err
_htmldoc_node_init_base_(HtmlDocNode n[static 1], HtmlDocNode* parent) {
    *n = (HtmlDocNode){0};
    n->childs = std_malloc(sizeof *n->childs);
    if (!n->childs) return "error: htmldoc_doc mem failure";
    *n->childs = (ArlOf(HtmlDocNode)){0};
    n->parent = parent;
    return Ok;
}

static inline Err
htmldoc_node_init_from_curlu(
    HtmlDocNode n[static 1], HtmlDocNode* parent, CURLU* cu, UrlClient url_client[static 1]
) {
    try(_htmldoc_node_init_base_(n, parent));
    try( htmldoc_init_fetch_browse_from_curlu(htmldoc_node_doc(n), cu, url_client));
    n->current_ix = n->childs->len;
    return Ok;
}

static inline Err
htmldoc_node_init(
    HtmlDocNode n[static 1], HtmlDocNode* parent, const char* url, UrlClient url_client[static 1]
) {
    try(_htmldoc_node_init_base_(n, parent));
    //try( htmldoc_init_fetch_browse(htmldoc_node_doc(n), url, url_client));
    Err e = htmldoc_init_fetch_browse(htmldoc_node_doc(n), url, url_client);
    if (e) {
        htmldoc_node_cleanup(n);
        return e;
    }
    n->current_ix = n->childs->len;
    return Ok;
}


/* Tree */
/* getters */
static inline HtmlDocNode* htmldoc_tree_head(HtmlDocTree t[static 1]) { return &t->head; }
static inline HtmlDoc* htmldoc_tree_head_doc(HtmlDocTree t[static 1]) {
    return &htmldoc_tree_head(t)->doc;
}
static inline Err htmldoc_tree_current_doc(HtmlDocTree t[static 1], HtmlDoc* out[static 1]) {
    try( htmldoc_node_current_doc(htmldoc_tree_head(t), out));
    return Ok;

}

static inline Err htmldoc_tree_current_node(HtmlDocTree t[static 1], HtmlDocNode* out[static 1]) {
    return htmldoc_node_current_node(htmldoc_tree_head(t), out);
    //try( htmldoc_node_current_node(htmldoc_tree_head(t), out));
    //return Ok;
}


/* ctor */
static inline Err
htmldoc_tree_init(HtmlDocTree t[static 1], const char* url, UrlClient url_client[static 1]) {
    *t = (HtmlDocTree){0};
    try( htmldoc_node_init(htmldoc_tree_head(t), 0x0, url, url_client));
    return Ok;
}

/* dtor */
void htmldoc_tree_cleanup(HtmlDocTree t[static 1]);

/**/
static inline
Err htmldoc_tree_append_ahref(HtmlDocTree t[static 1], size_t linknum, UrlClient url_client[static 1])
{
    HtmlDocNode* n;
    try(  htmldoc_tree_current_node(t, &n));
    if (!n) return "error: current node not found";
    HtmlDoc* d = &n->doc;
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, linknum);
    if (!a) return "link number invalid";

    CURLU* curlu = url_cu(htmldoc_url(d));
    try( lexcurl_dup_curl_with_anchors_href(*a, &curlu));

    HtmlDocNode newnode;
    try( htmldoc_node_init_from_curlu(&newnode, n, curlu, url_client));
    Err err;
    if ((err=htmldoc_node_append_move_child(n, &newnode))) {
        curl_url_cleanup(curlu);
        return err;
    }
    //if (!arlfn(HtmlDocNode, append)(htmldoc_node_childs(n), &newnode))
    //    return "error: mem failure arl";

    //n->current_ix = htmldoc_node_child_count(n);
    //if (!n->current_ix) return "error: no items after appending";
    //--n->current_ix; /* the iux of last one */
    return Ok;
}

static inline
Err htmldoc_tree_append_submit(HtmlDocTree t[static 1], size_t ix, UrlClient url_client[static 1])
{
    HtmlDocNode* n;
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

        HtmlDocNode newnode;
        try( htmldoc_node_init_from_curlu(&newnode, n, curlu, url_client));
        if ((err=htmldoc_node_append_move_child(n, &newnode))) {
            curl_url_cleanup(curlu);
            return err;
        }
    } else { puts("expected form, not found"); }
    return Ok;
}

static inline
Err htmldoc_tree_append_url(HtmlDocTree t[static 1], const char* url) {
    HtmlDocNode* cn;
    try( htmldoc_tree_current_node(t, &cn));
    if (!cn) { /* head is current node */
        cn = htmldoc_tree_head(t);
    }

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

    size_t len = htmldoc_node_childs(cn)->len;
    if (!len) return "error: no childs after append";
    cn->current_ix = len - 1;
    return Ok;
}

static inline 
Err dbg_htmldoc_node_print(HtmlDocNode n[static 1], size_t ix, size_t h) {
    HtmlDoc* d = &n->doc;
    LxbNodePtr node = *htmldoc_title(d);
    if (h) {
        if (2*h > INT_MAX) return "error: well that's a large tree";
        printf("%*c", (int)(2*h), ' ');
    }
    printf("%ld : ", ix);
    try( dbg_print_title(node));

    HtmlDocNode* it = arlfn(HtmlDocNode, begin)(n->childs);
    //const HtmlDocNode* beg = it;
    const HtmlDocNode* end = arlfn(HtmlDocNode, end)(n->childs);
    for (; it != end; ++it) {
        try( dbg_htmldoc_node_print(it, ix, h+1));
    }
    return Ok;
}
#endif
