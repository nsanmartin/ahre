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
    TabNode n[static 1], TabNode* parent, CURLU* cu, UrlClient url_client[static 1], HttpMethod method
) {
    try(_tab_node_init_base_(n, parent));
    try( htmldoc_init_fetch_browse_from_curlu(tab_node_doc(n), cu, url_client, method));
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


////* Tree */
////* getters */
///static inline TabNode* htmldoc_tree_head(TabNode t[static 1]) { return &t->head; }
///static inline HtmlDoc* htmldoc_tree_head_doc(TabNode t[static 1]) {
///    return &htmldoc_tree_head(t)->doc;
///}
///static inline Err htmldoc_tree_current_doc(TabNode t[static 1], HtmlDoc* out[static 1]) {
///    try( tab_node_current_doc(htmldoc_tree_head(t), out));
///    return Ok;
///
///}
///
///static inline Err tab_node_current_node(TabNode t[static 1], TabNode* out[static 1]) {
///    return tab_node_current_node(htmldoc_tree_head(t), out);
///}
///
///
////* ctor */
///static inline Err
///htmldoc_tree_init(TabNode t[static 1], const char* url, UrlClient url_client[static 1]) {
///    *t = (TabNode){0};
///    try( tab_node_init(htmldoc_tree_head(t), 0x0, url, url_client));
///    return Ok;
///}
///
////* dtor */
///void htmldoc_tree_cleanup(TabNode t[static 1]);

/**/
static inline
Err htmldoc_tree_append_ahref(TabNode t[static 1], size_t linknum, UrlClient url_client[static 1])
{
    TabNode* n;
    try(  tab_node_current_node(t, &n));
    if (!n) return "error: current node not found";
    HtmlDoc* d = &n->doc;
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, linknum);
    if (!a) return "link number invalid";

    CURLU* curlu = url_cu(htmldoc_url(d));
    try( lexcurl_dup_curl_with_anchors_href(*a, &curlu));

    TabNode newnode;
    try( tab_node_init_from_curlu(&newnode, n, curlu, url_client, http_get));
    Err err;
    if ((err=tab_node_append_move_child(n, &newnode))) {
        curl_url_cleanup(curlu);
        return err;
    }
    return Ok;
}

static inline
Err htmldoc_tree_append_submit(TabNode t[static 1], size_t ix, UrlClient url_client[static 1])
{
    TabNode* n;
    try(  tab_node_current_node(t, &n));
    if (!n) return "error: current node not found";

    HtmlDoc* d;
    try(  tab_node_current_doc(t, &d));
    if (!d) return "error: current doc not found";

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
        try( tab_node_init_from_curlu(&newnode, n, curlu, url_client, method));
        if ((err=tab_node_append_move_child(n, &newnode))) {
            curl_url_cleanup(curlu);
            return err;
        }
    } else { puts("expected form, not found"); }
    return Ok;
}

static inline
Err htmldoc_tree_append_url(TabNode t[static 1], const char* url) {
    TabNode* cn;
    try( tab_node_current_node(t, &cn));
    if (!cn) { /* head is current node */
        cn = t;
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
Err dbg_tab_node_print(TabNode n[static 1], size_t ix, ArlOf(size_t) stack[static 1]) {
    if (!arlfn(size_t, append)(stack, &ix)) {
        arlfn(size_t, clean)(stack);
        return "error: arl append failure";
    }
    HtmlDoc* d = &n->doc;
    LxbNodePtr node = *htmldoc_title(d);
    for(size_t* it = arlfn(size_t, begin)(stack); it != arlfn(size_t, end)(stack); ++it) {
        printf(".%ld", *it);
    }
    printf("%s", ": ");
    try( dbg_print_title(node));

    TabNode* it = arlfn(TabNode, begin)(n->childs);
    const TabNode* beg = it;
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) {
        size_t subix = it-beg;
        try( dbg_tab_node_print(it, subix, stack));
    }
    if (!arlfn(size_t, pop)(stack)) {
        arlfn(size_t, clean)(stack);
        return "error: arl pop failure";
    }
    return Ok;
}

static inline void tab_node_set_as_current(TabNode n[static 1]) {
    n->current_ix = tab_node_child_count(n);
}

static inline Err tab_node_move(TabNode tn[static 1], const char* line) {
    size_t ix;
    try( parse_size_t_or_throw(&line, &ix, 10));
    tn = arlfn(TabNode,at)(tab_node_childs(tn), ix);
    if (!tn) return "invalid tab child";
    line = cstr_skip_space(line);
    if (!*line) {
        tab_node_set_as_current(tn);
        return Ok;
    }
    if (*line != '.') return "invalid tab path";
    return tab_node_move(tn, line + 1);
}
#endif
