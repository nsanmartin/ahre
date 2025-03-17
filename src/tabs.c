#include "src/tabs.h"
#include "src/tab_node.h"
#include "src/session.h"

Err tab_node_init(
    TabNode n[static 1],
    TabNode* parent,
    const char* url,
    UrlClient url_client[static 1],
    Session s[static 1]
);
Err tablist_append_tree_from_url(
    TabList f[static 1],
    const char* url,
    UrlClient url_client[static 1],
    Session s[static 1]
) {
    TabNode tn = (TabNode){0};
    try( tab_node_init(&tn, 0x0, url, url_client, s));
    Err err = tablist_append_move_tree(f, &tn);
    if (err) {
        tab_node_cleanup(&tn);
        return err;
    }
    if (!f->tabs.len) return "error: expecting tabs in the tab list after appending a tab";
    f->current_tab = f->tabs.len - 1;
    return Ok;
}


