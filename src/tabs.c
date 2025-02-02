#include "src/tabs.h"


void arl_of_htmldoc_tree_clean(ArlOf(TabNode)* t) {
    //TODO: fix #inclkude TClean <- arlfn(TabNode, clean)(t);

    for (TabNode* it = arlfn(TabNode, begin)(t); it != arlfn(TabNode,end)(t); ++it) {
        tab_node_cleanup(it);
    }
    std_free((void*)t->items);
    *t = (ArlOf(TabNode)){0};
}

