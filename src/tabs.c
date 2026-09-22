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


static Err
tab_node_enumerated_get_node(TabNode n[_1_], size_t ix[_1_], TabNodePtr out[_1_]) {
    if (!*ix) {
        *out = n;
        return Ok;
    }

    --*ix;


    TabNode* it        = arlfn(TabNode, begin)(n->childs);
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) {
        try( tab_node_enumerated_get_node(it, ix, out));
        if (!*ix && *out) break;
    }

    return Ok;
}


static Err
tablist_enumerated_get_node(
    TabList f[_1_], size_t ix[_1_], TabNodePtr out[_1_], size_t tablist_ix[_1_]
) {

    if (*out) fail_e("tablist_enumerated_get_node: out ptr must be null");
    TabNode* it        = arlfn(TabNode, begin)(&f->tabs);
    const TabNode* beg = it;
    const TabNode* end = arlfn(TabNode, end)(&f->tabs);
    
    for (; it != end; ++it) {
        *tablist_ix = it - beg;
        try(tab_node_enumerated_get_node(it, ix, out));
        if (!*ix && *out) break;
    }

    if (*ix || !*out) return "not tab with given index";
    return Ok;
}



Err
tablist_move_to_node(TabList tl[_1_], const char* line) {
    line = cstr_skip_space(line);
    if (!*line) fail_e("expecting non empty line");
    size_t ix;
    const char* endptr = NULL;
    try( parse_size_t_err(line, &ix, &endptr, 36));
    if (endptr && *endptr) return "invalid tab index";

    TabNode* search = NULL;
    size_t tablist_ix;
    try(tablist_enumerated_get_node(tl, &ix, &search, &tablist_ix));
    try( tab_node_set_as_current(search));
    *_tablist_current_tab_ix_(tl) = tablist_ix;
    return Ok;

}


static Err
tablist_to_node_list(TabNode n[_1_], ArlOf(TabNodePtr) nodes[_1_], CmdOut* out) {
    if (!arlfn(TabNodePtr,append)(nodes,&n)) fail_e("arl append");

    TabNode* it        = arlfn(TabNode, begin)(n->childs);
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) 
        try( tablist_to_node_list(it, nodes, out));

    return Ok;
}


Err
tablist_info_titles(TabList f[_1_], CmdOut* out) {
    Str buf                  = (Str){0};
    ArlOf(TabNodePtr)* nodes = &(ArlOf(TabNodePtr)){0};

    Err err = Ok;

    TabNode* it          = arlfn(TabNode, begin)(&f->tabs);
    const TabNode* end   = arlfn(TabNode, end)(&f->tabs);
    
    TabNode* current_node = NULL;
    tryjmp(err,Clean, tablist_current_node(f, &current_node));
    size_t prev_offset = 0;

    for (; it != end; ++it)  {
        prev_offset += len__(nodes);
        arlfn(TabNodePtr,reset)(nodes);
        tryjmp(err,Clean, msg__(out, "    .\n"));
        tryjmp(err,Clean, tablist_to_node_list(it, nodes, out));

        const TabNodePtr* nodes_offset = arlfn(TabNodePtr,begin)(nodes);
        foreach__(TabNodePtr,nodes,n) {
            if (*n == current_node) tryjmp(err,Clean, msg__(out, "[*] "));
            else if (tab_node_is_current_in_tab(*n)) tryjmp(err,Clean, msg__(out, "[ ] "));
            else tryjmp(err,Clean, msg__(out, "    "));
            const size_t ix = prev_offset + n - nodes_offset;
            tryjmp(err,Clean, cmd_out_msg_append_ui_as_base36(out, ix));
            tryjmp(err,Clean, msg__(out, " "));
            str_reset(&buf);
            tryjmp(err,Clean, tab_node_to_bookmark_description(*n, &buf));
            tryjmp(err,Clean, msg_ln__(out, buf));
        }
    }

Clean:
    arlfn(TabNodePtr, clean)(nodes);
    str_clean(&buf);
    return Ok;
}


Err tablist_info_tree(TabList f[_1_], CmdOut* out) {
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
