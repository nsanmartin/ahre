#include "src/htmldoc_forest.h"


void arl_of_htmldoc_tree_clean(ArlOf(HtmlDocTree)* t) {
    //TODO: fix #inclkude TClean <- arlfn(HtmlDocTree, clean)(t);

    for (HtmlDocTree* it = arlfn(HtmlDocTree, begin)(t); it != arlfn(HtmlDocTree,end)(t); ++it) {
        htmldoc_tree_cleanup(it);
    }
    std_free((void*)t->items);
    *t = (ArlOf(HtmlDocTree)){0};
}

