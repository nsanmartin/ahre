/* Copyright (C) 2026 Nicolas San Martin

This file is part of ahre.

ahre is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

ahre is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ahre; see the file LICENSE or see
<https://www.gnu.org/licenses/>.  */

#include "tab-node.h"
#include "session.h"
#include "dom.h"
#include "generic.h"


/* ctor */
static Err
_tab_node_init_base_(TabNode n[_1_], TabNode* parent) {
    *n = (TabNode){0};
    n->childs = std_malloc(sizeof *n->childs);
    if (!n->childs) return "error: tab_node_init_from_cstr mem failure";

    *n->childs = (ArlOf(TabNode)){0};
    n->parent  = parent;
    return Ok;
}


Err tab_node_init_move_request(
    TabNode     n[_1_],
    TabNode*    parent,
    UrlClient   url_client[_1_],
    Request     r[_1_],
    Session*     s,
    CmdOut*     out
) {
    try(_tab_node_init_base_(n, parent));
    try( htmldoc_ptr_init_move_request(tab_node_doc(n), r, url_client, s, out));

    n->current_ix = tab_node_child_count(n);
    return Ok;
}


Err tab_node_tree_append_ahref_from_node(
    TabNode   t[_1_],
    DomNode   a,
    UrlClient url_client[_1_],
    Session*  s,
    bool      cache_req,
    CmdOut*   out
) {
    if (!s) return "error: expecting a session";
    TabNode* n;
    HtmlDocPtr* d;
    try( tab_node_current_node(t, &n));
    try( tab_node_current_doc(t, &d));

    Err     e       = Ok;
    StrView html    = dom_node_attr_value(a, svl("href"));
    Request r       = (Request){0};
    TabNode newnode = (TabNode){0};

    tryjmp(e,Clean, request_init_get(&r, html, htmldoc_url(d->ptr), cache_req));
    tryjmp(e,Clean, tab_node_init_move_request(&newnode, n, url_client, &r, s, out));
    tryjmp(e,Clean, tab_node_append_move_child(n, &newnode));
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
    HtmlDocPtr* d;
    try( tab_node_current_node(t, &tab_node));
    try( tab_node_current_doc(t, &d));

    DomNode form = dom_node_find_parent_form(input_node);
    if (isnull(form)) { return "expected form, not found"; }

    Request r       = (Request){0};
    TabNode newnode = (TabNode){0};
    Err e = Ok;
    tryjmp(e,Clean,
        request_from_form_node(&r, form, htmldoc_url(d->ptr), htmldoc_checked_boxes(d->ptr)));
    tryjmp(e,Clean, tab_node_init_move_request(&newnode, tab_node, url_client, &r, s, out));
    tryjmp(e,Clean, tab_node_append_move_child(tab_node, &newnode));
    return Ok;

Clean:
    request_clean(&r);
    tab_node_cleanup(&newnode);
    return e;
}


void
tab_node_cleanup(TabNode n[_1_]) {
    htmldoc_destroy(n->doc.ptr);
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

Err tab_node_to_bookmark_description(TabNode n[_1_], Str description[_1_]) {
    DomNode title;
    HtmlDoc* d = n->doc.ptr;
    try(htmldoc_title(d, &title));
    if (!isnull(title))
        try(strview_join_lines_to_str(
            dom_node_text_view(dom_node_first_child(title)), description));
    else {
        char* buf;
        try(url_cstr_malloc(*htmldoc_url(d), &buf));
        Err e = str_append(description, sv(buf));
        w_curl_free(buf);
        try(e);
    }
    return Ok;
}


Err session_tab_node_print(
    TabNode n[_1_],
    size_t ix,
    ArlOf(size_t) stack[_1_],
    TabNode* current_node,
    CmdOut* out
) {
    if (!arlfn(size_t, append)(stack, &ix)) return "error: arl append failure";

    for(size_t* it = arlfn(size_t, begin)(stack); it != arlfn(size_t, end)(stack); ++it) {
        if (it == arlfn(size_t, begin)(stack)) {
            if (n == current_node) {
                try( msg__(out, svl("[+].")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
            } else if (tab_node_is_current_in_tab(n)) {
                try( msg__(out, svl("[ ].")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
            } else {
                try( msg__(out, svl("   .")));
                try( cmd_out_msg_append_ui_as_base10(out, *it));
            }
        } else { 
            try( msg__(out, svl(".")));
            try( cmd_out_msg_append_ui_as_base10(out, *it));
        }
    }
    try( msg__(out, svl(" ")));
    try(tab_node_to_bookmark_description(n, msg_str(cmd_out_msg(out))));
    try( msg__(out, svl("\n")));

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


Err
tab_node_current_doc(TabNode n[_1_], HtmlDocPtr* out[_1_] ) {
    TabNode* cn;
    try( tab_node_current_node(n, &cn));
    if (!cn) return "error: current node in NULL";
    *out = &cn->doc;
    return Ok;
}

size_t
tab_node_count(TabNode n[_1_]) {
    size_t count = 1;
    foreach__(TabNode,tab_node_childs(n),it) {
        count += tab_node_count(it);
    }
    return count;
}


Err
tab_node_current_node_ix(TabNode n[_1_], size_t out[_1_]) {
    TabNode* prev = NULL;

    while (n) {
        prev = n;
        n = arlfn(TabNode, at)(n->childs, n->current_ix);
        if (n) {
            ++*out;
            for (size_t ix = 0; ix < prev->current_ix; ++ix)
                *out += tab_node_count(arlfn(TabNode,at)(tab_node_childs(prev),ix));
        }
    }

    if (!prev) return "error: tab_node_current_node failure";
    return Ok;
}

Err
tab_node_show_title_cmd(CmdParams p[_1_], TabNode n[_1_]) {
    return tab_node_to_bookmark_description(n, cmd_out_std(cmd_params_cmd_out(p)));
}
