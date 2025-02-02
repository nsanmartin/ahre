#include "src/htmldoc_tree.h"

void arl_of_htmldoc_node_clean(ArlOf(TabNode)* f) {
    //TODO: fix #include TClean <- ...

    for (TabNode* it = arlfn(TabNode, begin)(f); it != arlfn(TabNode,end)(f); ++it) {
        tab_node_cleanup(it);
    }
    std_free((void*)f->items);
    *f = (ArlOf(TabNode)){0};
}


void tab_node_cleanup(TabNode n[static 1]) {
    htmldoc_cleanup(&n->doc);
    //arlfn(TabNode, clean)(n->childs);
    arl_of_htmldoc_node_clean(n->childs);
    std_free(n->childs);
}


void htmldoc_tree_cleanup(HtmlDocTree t[static 1]) {
    tab_node_cleanup(&t->head);
    *t = (HtmlDocTree){0};
}


