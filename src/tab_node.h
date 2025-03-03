#ifndef AHRE_TAB_NODE_H__
#define AHRE_TAB_NODE_H__

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
    if (!n->childs) return "error: tab_node_init mem failure";
    *n->childs = (ArlOf(TabNode)){0};
    n->parent = parent;
    return Ok;
}

static inline Err
tab_node_init_from_curlu(
    TabNode n[static 1],
    TabNode* parent,
    CURLU* cu,
    UrlClient url_client[static 1],
    HttpMethod method,
    SessionConf sconf[static 1]
) {
    try(_tab_node_init_base_(n, parent));
    Err err = htmldoc_init_fetch_draw_from_curlu(tab_node_doc(n), cu, url_client, method, sconf);
    if (err) {
        /* cu is owned by caller so if an erro occur we must not free it but him */
        htmldoc_url(tab_node_doc(n))->cu = NULL;
        tab_node_cleanup(n);
        return err;
    }
    n->current_ix = n->childs->len;
    return Ok;
}

static inline Err
tab_node_init(
    TabNode n[static 1],
    TabNode* parent,
    const char* url,
    UrlClient url_client[static 1],
    SessionConf sconf[static 1]
) {
    try(_tab_node_init_base_(n, parent));
    Err e = htmldoc_init_fetch_draw(tab_node_doc(n), url, url_client, sconf);
    if (e) {
        /* doc if initialized was cleaned by htmldoc_init_fetch_draw. */
        *tab_node_doc(n) = (HtmlDoc){0};
        tab_node_cleanup(n);
        return e;
    }
    n->current_ix = n->childs->len;
    return Ok;
}


/**/
static inline
Err tab_node_tree_append_ahref(
    TabNode t[static 1],
    size_t linknum,
    UrlClient url_client[static 1],
    SessionConf sconf[static 1]
) {
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
    Err err;
    if((err=tab_node_init_from_curlu(&newnode, n, curlu, url_client, http_get, sconf))) {
        curl_url_cleanup(curlu);
        return err;
    };

    if ((err=tab_node_append_move_child(n, &newnode))) {
        tab_node_cleanup(&newnode);
        return err;
    }
    return Ok;
}

static inline
Err tab_node_tree_append_submit(
    TabNode t[static 1],
    size_t ix,
    UrlClient url_client[static 1],
    SessionConf sconf[static 1]
) {
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
        if ((err=tab_node_init_from_curlu(&newnode, n, curlu, url_client, method, sconf))) {
            curl_url_cleanup(curlu);
            return err;
        };
        if ((err=tab_node_append_move_child(n, &newnode))) {
            curl_url_cleanup(curlu);
            return err;
        }
    } else { puts("expected form, not found"); }
    return Ok;
}

static inline
Err tab_node_tree_append_url(TabNode t[static 1], const char* url) {
    TabNode* cn;
    try( tab_node_current_node(t, &cn));
    if (!cn) { /* head is current node */
        cn = t;
    }

    TabNode new_node = (TabNode){.parent=cn};
    try( htmldoc_init(&new_node.doc, url));
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

static inline 
Err dbg_tab_node_print(
    TabNode n[static 1], size_t ix, ArlOf(size_t) stack[static 1], TabNode* current_node
) {
    if (!arlfn(size_t, append)(stack, &ix)) {
        arlfn(size_t, clean)(stack);
        return "error: arl append failure";
    }
    HtmlDoc* d = &n->doc;
    LxbNodePtr title = *htmldoc_title(d);

    for(size_t* it = arlfn(size_t, begin)(stack); it != arlfn(size_t, end)(stack); ++it) {
        if (it == arlfn(size_t, begin)(stack)) {
            if (n == current_node) printf("[+] %ld.", *it);
            else if (tab_node_is_current_in_tab(n)) printf("[ ] %ld.", *it);
            else printf("    %ld.", *it);
        } else printf("%ld.", *it);
    }
    printf("%s", " ");
    if (title) try( dbg_print_title(title));
    else {
        char* buf;
        Err e = url_cstr(htmldoc_url(d), &buf);
        if (e) printf("error: %s\n", e);
        else {
            printf("%s\n", buf);
            curl_free(buf);
        }
    }

    TabNode* it = arlfn(TabNode, begin)(n->childs);
    const TabNode* beg = it;
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) {
        size_t subix = it-beg;
        try( dbg_tab_node_print(it, subix, stack, current_node));
    }
    if (!stack->len) {
        arlfn(size_t, clean)(stack);
        return "error: arl pop failure";
    } else --stack->len;
    return Ok;
}

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
