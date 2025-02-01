#include "src/htmldoc_tree.h"

void arl_of_htmldoc_node_clean(ArlOf(HtmlDocNode)* f) {
    //TODO: fix #include TClean <- ...

    for (HtmlDocNode* it = arlfn(HtmlDocNode, begin)(f); it != arlfn(HtmlDocNode,end)(f); ++it) {
        htmldoc_node_cleanup(it);
    }
    std_free((void*)f->items);
    *f = (ArlOf(HtmlDocNode)){0};
}


void htmldoc_node_cleanup(HtmlDocNode n[static 1]) {
    htmldoc_cleanup(&n->doc);
    //arlfn(HtmlDocNode, clean)(n->childs);
    arl_of_htmldoc_node_clean(n->childs);
    std_free(n->childs);
}


void htmldoc_tree_cleanup(HtmlDocTree t[static 1]) {
    htmldoc_node_cleanup(&t->head);
    *t = (HtmlDocTree){0};
}


