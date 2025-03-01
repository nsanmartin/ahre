#include "src/tab_node.h"

void arl_of_tab_node_clean(ArlOf(TabNode)* f) {
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
    arl_of_tab_node_clean(n->childs);
    std_free(n->childs);
}

void _set_parent_to_all_(TabNode parent[static 1], ArlOf(TabNode) childs[static 1]) {
    for (TabNode* it = arlfn(TabNode,begin)(childs); it != arlfn(TabNode,end)(childs); ++it) {
        it->parent = parent;
    }
}

void _fix_childs_parents_(ArlOf(TabNode)* list) {
    for (TabNode* it = arlfn(TabNode,begin)(list); it != arlfn(TabNode,end)(list); ++it) {
        _set_parent_to_all_(it, tab_node_childs(it));
    }
}

TabNode* arl_of_tab_node_append(ArlOf(TabNode)* list, TabNode tn[static 1]) {
    TabNode* res = NULL;
    TabNode* mem_location = list->items;
    res = arlfn(TabNode, append)(list, tn);
    if (!res) return res;
    if (mem_location && mem_location != list->items) { _fix_childs_parents_(list); }
    return res;
}
