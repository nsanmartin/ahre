#include "tab-node.h"
#include "session.h"
#include "dom.h"
#include "generic.h"


Err tab_node_init_move_request(
    TabNode     n[_1_],
    TabNode*    parent,
    UrlClient   url_client[_1_],
    Request     r[_1_],
    Session*     s,
    CmdOut*     out
) {
    try(_tab_node_init_base_(n, parent));
    try( htmldoc_init_move_request(tab_node_doc(n), r, url_client, s, out));

    n->current_ix = n->childs->len;
    return Ok;
}


Err tab_node_tree_append_ahref_from_node(
    TabNode   t[_1_],
    DomNode   a,
    UrlClient url_client[_1_],
    Session*  s,
    CmdOut*   out
) {
    if (!s) return "error: expecting a session";
    TabNode* n;
    HtmlDoc* d;
    try( tab_node_current_node(t, &n));
    try( tab_node_current_doc(t, &d));

    Err     e       = Ok;
    StrView html    = dom_node_attr_value(a, svl("href"));
    Request r       = (Request){0};
    TabNode newnode = (TabNode){0};

    try_or_jump(e,Clean, request_init(&r,http_get, html, htmldoc_url(d)));
    try_or_jump(e,Clean, tab_node_init_move_request(&newnode, n, url_client, &r, s, out));
    try_or_jump(e,Clean, tab_node_append_move_child(n, &newnode));
    return Ok;
Clean:
    tab_node_cleanup(&newnode);
    request_clean(&r);
    return e;
}


Err tab_node_tree_append_submit_input_node(
    TabNode t[_1_],
    DomNode  input_node,
    UrlClient url_client[_1_],
    Session s[_1_],
    CmdOut* out
) {
    TabNode* tab_node;
    HtmlDoc* d;
    try( tab_node_current_node(t, &tab_node));
    try( tab_node_current_doc(t, &d));

    DomNode form = dom_node_find_parent_form(input_node);
    if (isnull(form)) { return "expected form, not found"; }

    Request r       = (Request){0};
    TabNode newnode = (TabNode){0};
    Err e = Ok;
    try_or_jump(e,Clean, request_from_form_node(&r, form, true, htmldoc_url(d)));
    try_or_jump(e,Clean, tab_node_init_move_request(&newnode, tab_node, url_client, &r, s, out));
    try_or_jump(e,Clean, tab_node_append_move_child(tab_node, &newnode));
    return Ok;

Clean:
    request_clean(&r);
    tab_node_cleanup(&newnode);
    return e;
}


void tab_node_cleanup(TabNode n[_1_]) {
    htmldoc_cleanup(&n->doc);
    if (n->childs) arlfn(TabNode, clean)(n->childs);
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
                try( msg__(out, svl("[+] ")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( msg__(out, svl(".")));
            } else if (tab_node_is_current_in_tab(n)) {
                try( msg__(out, svl("[ ] ")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( msg__(out, svl(".")));
            } else {
                try( msg__(out, svl("    ")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( msg__(out, svl(".")));
            } } else { 
                try( cmd_out_msg_append_ui_as_base10(out, *it));
                try( msg__(out, svl(".")));
            }
    }
    try( msg__(out, svl(" ")));
    DomNode title = *htmldoc_title(d);
    if (!isnull(title)) {
            Err e = strview_join_lines_to_str(
                dom_node_text_view(dom_node_first_child(title)),
                msg_str(cmd_out_msg(out))
            );
            ok_then(e, msg__(out, svl("\n")));
    } else {
        char* buf;
        Err e = url_cstr_malloc(*htmldoc_url(d), &buf);
        if (e) {
            try( msg__(out, svl("error: ")));
            try( msg__(out, (char*)e));
            try( msg__(out, svl("\n")));
        } else {
            e = msg__(out, buf);
            ok_then(e, msg__(out, svl("\n")));
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
