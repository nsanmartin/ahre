#include "tabs.h"
#include "generic.h"
#include "session.h"



Err tablist_append_tree_from_url(
    TabList     f[_1_],
    Request     r[_1_],
    UrlClient   url_client[_1_],
    Session     s[_1_],
    CmdOut      cout[_1_]
) {
    Err     e  = Ok;
    TabNode tn = (TabNode){0};
    tryjmp(e,Fail, tab_node_init_move_request(&tn, NULL, url_client, r, s, cout));

    tryjmp(e,Fail, tablist_append_move_tree(f, &tn));
    if (!f->tabs.len) {
        e = "error: expecting tabs in the tab list after appending a tab";
        goto Fail;
    }
    f->current_tab = f->tabs.len - 1;
    return Ok;
Fail:
    tab_node_cleanup(&tn);
    return e;
}


Err tablist_info(TabList f[_1_], CmdOut* out) {
    ArlOf(size_t)* stack = &(ArlOf(size_t)){0};

    TabNode* current_node;
    Err err = Ok;
    ok_then(err, tablist_current_node(f, &current_node));
    ok_then(err, msg__(out, svl("(")));
    ok_then(err, cmd_out_msg_append_ui_as_base10(out, f->tabs.len));
    ok_then(err, msg__(out, svl(" tab")));
    if(f->tabs.len) ok_then(err, msg__(out, svl("s")));
    ok_then(err, msg__(out, svl(")\n")));

    if (!err) {
        TabNode* it = arlfn(TabNode, begin)(&f->tabs);
        const TabNode* beg = it;
        const TabNode* end = arlfn(TabNode, end)(&f->tabs);
        
        for (; it != end; ++it) {
            size_t ix = it-beg;
            if ((err=session_tab_node_print(it, ix, stack, current_node, out))) break;
        }
    }
    arlfn(size_t, clean)(stack);
    ok_then(err, msg__(out, svl("\n")));
    return Ok;
}
