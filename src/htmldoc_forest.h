#ifndef AHRE_HTMLDOC_FOREST_H__
#define AHRE_HTMLDOC_FOREST_H__
#include "src/htmldoc_tree.h"

#define T HtmlDocTree
// #define TClean htmldoc_tree_cleanup
#include <arl.h>

void arl_of_htmldoc_tree_clean(ArlOf(HtmlDocTree)* t);

typedef struct {
    ArlOf(HtmlDocTree) trees;
    size_t current;
} HtmlDocForest;

#define mk_htmldoc_forest() (HtmlDocForest){0}

/* getter */
static inline HtmlDocTree* htmldoc_forest_current_tree(HtmlDocForest f[static 1]) {
    return arlfn(HtmlDocTree, at)(&f->trees, f->current);
}

static inline HtmlDoc* htmldoc_forest_current_doc(HtmlDocForest f[static 1]) {
    return htmldoc_tree_current_doc(htmldoc_forest_current_tree(f));
}

static inline Err
htmldoc_forest_append_move_tree(HtmlDocForest f[static 1], HtmlDocTree t[static 1]) {
    if (!arlfn(HtmlDocTree, append)(&f->trees, t)) return "error: arl append failure";
    return Ok;
}

/* ctor */
static inline Err htmldoc_forest_init(HtmlDocForest f[static 1], const char* url) {
    HtmlDocTree t = (HtmlDocTree){0};
    try( htmldoc_tree_init(&t));
    try( htmldoc_tree_append_url(&t, url));
    return htmldoc_forest_append_move_tree(f, &t);
}

/* dtor */
static inline void htmldoc_forest_cleanup(HtmlDocForest f[static 1]) {
    arl_of_htmldoc_tree_clean(&f->trees);
}
#endif
