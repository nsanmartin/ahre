#ifndef AHRE_HTMLDOC_FOREST_H__
#define AHRE_HTMLDOC_FOREST_H__
#include "src/htmldoc_tree.h"

#define T HtmlDocTree
// #define TClean htmldoc_tree_cleanup
#include <arl.h>

void arl_of_htmldoc_tree_clean(ArlOf(HtmlDocTree)* t);

typedef struct {
    ArlOf(HtmlDocTree) trees;
    size_t current_tree;
} HtmlDocForest;


/* getter */

static inline ArlOf(HtmlDocTree)* _htmldoc_forest_trees_(HtmlDocForest f[static 1]) {
    return &f->trees;
}

static inline size_t _htmldoc_forest_tree_count_(HtmlDocForest f[static 1]) {
    return _htmldoc_forest_trees_(f)->len;
}

static inline bool htmldoc_forest_is_empty(HtmlDocForest f[static 1]) {
    return _htmldoc_forest_tree_count_(f) == 0;
}

static inline Err
htmldoc_forest_current_tree(HtmlDocForest f[static 1], HtmlDocTree* out[static 1]) {
    HtmlDocTree* current_tree = arlfn(HtmlDocTree, at)(&f->trees, f->current_tree);
    if (!current_tree && !htmldoc_forest_is_empty(f)) return "error: not current tree available";
    *out = current_tree;
    return Ok;
}

static inline Err htmldoc_forest_current_doc(HtmlDocForest f[static 1], HtmlDoc* out[static 1]) {
    HtmlDocTree* t;
    try( htmldoc_forest_current_tree(f, &t));
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
htmldoc_forest_append_move_tree(HtmlDocForest f[static 1], HtmlDocTree t[static 1]) {
    if (!arlfn(HtmlDocTree, append)(&f->trees, t)) return "error: arl append failure";
    return Ok;
}

static inline Err htmldoc_forest_append_tree_from_url(
    HtmlDocForest f[static 1], const char* url, UrlClient url_client[static 1]
) {
    HtmlDocTree t = (HtmlDocTree){0};
    try( htmldoc_tree_init(&t, url, url_client));
    try( htmldoc_forest_append_move_tree(f, &t));
    if (!f->trees.len) return "error: expecting trees in the forest after appending a tree";
    f->current_tree = f->trees.len - 1;
    return Ok;
}


/* ctor */

static inline Err htmldoc_forest_init(
    HtmlDocForest f[static 1], const char* url, UrlClient url_client[static 1]
) {
    return htmldoc_forest_append_tree_from_url(f, url, url_client);
}

/* dtor */
static inline void htmldoc_forest_cleanup(HtmlDocForest f[static 1]) {
    arl_of_htmldoc_tree_clean(&f->trees);
}
#endif
