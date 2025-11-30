#include "tabs.h"
#include "tab-node.h"
#include "session.h"

Err tab_node_init_from_request(
    TabNode     n[static 1],
    TabNode*    parent,
    Url*        url,
    UrlClient   url_client[static 1],
    Request     r[static 1],
    Session     s[static 1]
);



Err tablist_append_tree_from_url(
    TabList     f[static 1],
    Request     r[static 1],
    UrlClient   url_client[static 1],
    Session     s[static 1]
) {
    TabNode tn = (TabNode){0};
    try( tab_node_init_from_request(&tn, NULL, NULL, url_client, r, s));

    Err err = Ok;
    try_or_jump(err, Failure_Tab_Node_Cleanup, tablist_append_move_tree(f, &tn));
    if (!f->tabs.len) {
        err = "error: expecting tabs in the tab list after appending a tab";
        goto Failure_Tab_Node_Cleanup;
    }
    f->current_tab = f->tabs.len - 1;
    return Ok;
Failure_Tab_Node_Cleanup:
    tab_node_cleanup(&tn);
    return err;
}


Err tablist_info(Session* s, TabList f[static 1]) {
    ArlOf(size_t)* stack = &(ArlOf(size_t)){0};

    TabNode* current_node;
    Err err = Ok;
    ok_then(err, tablist_current_node(f, &current_node));
    ok_then(err, session_write_msg_lit__(s, "("));
    ok_then(err, session_write_unsigned_msg(s, f->tabs.len));
    ok_then(err, session_write_msg_lit__(s, " tab"));
    if(f->tabs.len) ok_then(err, session_write_msg_lit__(s, "s"));
    ok_then(err, session_write_msg_lit__(s, ")\n"));

    if (!err) {
        TabNode* it = arlfn(TabNode, begin)(&f->tabs);
        const TabNode* beg = it;
        const TabNode* end = arlfn(TabNode, end)(&f->tabs);
        
        for (; it != end; ++it) {
            size_t ix = it-beg;
            if ((err=session_tab_node_print(s, it, ix, stack, current_node))) break;
        }
    }
    arlfn(size_t, clean)(stack);
    ok_then(err, session_write_msg_lit__(s, "\n"));
    return Ok;
}
