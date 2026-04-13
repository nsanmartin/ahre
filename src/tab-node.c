#include "tab-node.h"
#include "session.h"
#include "dom.h"


Err tab_node_init_move_request(
    TabNode     n[_1_],
    TabNode*    parent,
    UrlClient   url_client[_1_],
    Request     r[_1_],
    Session*     s,
    CmdOut*     out
) {
    try(_tab_node_init_base_(n, parent));
    Err err = htmldoc_init_move_request(tab_node_doc(n), r, url_client, s, out);
    if (err) {
        /* we don't want tab_node_cleanup to free node_doc again */
        *tab_node_doc(n) = (HtmlDoc){0};
        tab_node_cleanup(n);
        return err;
    }

    n->current_ix = n->childs->len;
    return Ok;
}


Err tab_node_tree_append_ahref(
    TabNode   t[_1_],
    size_t    linknum,
    UrlClient url_client[_1_],
    Session   s[_1_],
    CmdOut*   out
) {
    TabNode* n;
    HtmlDoc* d;
    try(  tab_node_current_node(t, &n));
    try(  tab_node_current_doc(t, &d));

    ArlOf(DomNode)* anchors = htmldoc_anchors(d);

    DomNode* a = arlfn(DomNode, at)(anchors, linknum);
    if (!a) return "link number invalid";

    Str urlstr   = (Str){0};
    StrView html = dom_node_attr_value(*a, svl("href"));
    try( str_append_z(&urlstr, &html));
    Request r;
    try(request_init_move_urlstr(&r,http_get, &urlstr, htmldoc_url(d)));

    TabNode newnode;
    Err e = Ok;
    try_or_jump(e, Clean_Url_Str,
        tab_node_init_move_request(&newnode, n, url_client, &r, s, out));
    try_or_jump(e, Failure_New_Node_Cleanup, tab_node_append_move_child(n, &newnode));
    goto Clean_Url_Str;
Failure_New_Node_Cleanup:
    tab_node_cleanup(&newnode);
Clean_Url_Str:
    request_clean(&r);
    return e;
}


Err tab_node_tree_append_submit(
    TabNode t[_1_],
    size_t ix,
    UrlClient url_client[_1_],
    Session s[_1_],
    CmdOut* out
) {
    TabNode* n;
    HtmlDoc* d;
    DomNode  lbn;
    try( tab_node_current_node(t, &n));
    try( tab_node_current_doc(t, &d));
    try( htmldoc_input_at(d, ix, &lbn));
    if (!dom_node_attr_has_value(lbn, svl("type"), svl("submit")))
        return "warn: not submit input";

    Err e = Ok;
    DomNode form = dom_node_find_parent_form(lbn);
    if (!isnull(form)) {
        Request r = (Request){.urlview=htmldoc_url(d)};
        try( mk_submit_request(form, true, &r));
        TabNode newnode;
        try_or_jump(e, Failure_Clean_Request,
            tab_node_init_move_request(&newnode, n, url_client, &r, s, out));
        try_or_jump(e, Failure_New_Node_Cleanup, tab_node_append_move_child(n, &newnode));
        return Ok;

Failure_Clean_Request:
        request_clean(&r);
        return e;
Failure_New_Node_Cleanup:
        tab_node_cleanup(&newnode);
        return e;

    } else { return "expected form, not found"; }

}


void tab_node_cleanup(TabNode n[_1_]) {
    htmldoc_cleanup(&n->doc);
    arlfn(TabNode, clean)(n->childs);
    std_free(n->childs);
}

static void _set_parent_to_all_(TabNode parent[_1_], ArlOf(TabNode) childs[_1_]) {
    for (TabNode* it = arlfn(TabNode,begin)(childs); it != arlfn(TabNode,end)(childs); ++it) {
        it->parent = parent;
    }
}

static void _fix_childs_parents_(ArlOf(TabNode)* list) {
    for (TabNode* it = arlfn(TabNode,begin)(list); it != arlfn(TabNode,end)(list); ++it) {
        _set_parent_to_all_(it, tab_node_childs(it));
    }
}

TabNode* arl_of_tab_node_append(ArlOf(TabNode)* list, TabNode tn[_1_]) {
    TabNode* res = NULL;
    TabNode* mem_location = list->items;
    res = arlfn(TabNode, append)(list, tn);
    if (!res) return res;
    if (mem_location && mem_location != list->items) { _fix_childs_parents_(list); }
    return res;
}

Err session_tab_node_print(
    TabNode n[_1_],
    size_t ix,
    ArlOf(size_t) stack[_1_],
    TabNode* current_node,
    CmdOut* out
) {
    if (!arlfn(size_t, append)(stack, &ix)) return "error: arl append failure";
    HtmlDoc* d = &n->doc;

    for(size_t* it = arlfn(size_t, begin)(stack); it != arlfn(size_t, end)(stack); ++it) {
        if (it == arlfn(size_t, begin)(stack)) {
            if (n == current_node) {
                try( cmd_out_msg_append(out, svl("[+] ")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( cmd_out_msg_append(out, svl(".")));
            } else if (tab_node_is_current_in_tab(n)) {
                try( cmd_out_msg_append(out, svl("[ ] ")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( cmd_out_msg_append(out, svl(".")));
            } else {
                try( cmd_out_msg_append(out, svl("    ")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( cmd_out_msg_append(out, svl(".")));
            } } else { 
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( cmd_out_msg_append(out, svl(".")));
            }
    }
    try( cmd_out_msg_append(out, svl(" ")));
    DomNode title = *htmldoc_title(d);
    if (!isnull(title)) {
            Err e = strview_join_lines_to_str(
                dom_node_text_view(dom_node_first_child(title)),
                msg_str(cmd_out_msg(out))
            );
            ok_then(e, cmd_out_msg_append(out, svl("\n")));
    } else {
        char* buf;
        Err e = url_cstr_malloc(*htmldoc_url(d), &buf);
        if (e) {
            try( cmd_out_msg_append(out, svl("error: ")));
            try( cmd_out_msg_append(out, (char*)e));
            try( cmd_out_msg_append(out, svl("\n")));
        } else {
            e = cmd_out_msg_append(out, buf);
            ok_then(e, cmd_out_msg_append(out, svl("\n")));
            w_curl_free(buf);
            if (e) return e;
        }
    }

    TabNode* it = arlfn(TabNode, begin)(n->childs);
    const TabNode* beg = it;
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) {
        size_t subix = it-beg;
        try( session_tab_node_print(it, subix, stack, current_node, out));
    }
    if (!stack->len) return "error: arl pop failure";
    else --stack->len;
    return Ok;
}
