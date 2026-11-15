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


Err
tablist_move_to_node_ix(TabList tl[_1_], size_t ix) {

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
tablist_info_titles_range(CmdParams p[_1_]) {
    CmdOut* out              = cmd_params_cmd_out(p);
    Str buf                  = (Str){0};
    ArlOf(TabNodePtr)* nodes = &(ArlOf(TabNodePtr)){0};
    TabList* f               = session_tablist(p->s);
    Err err                  = Ok;

    Range r;
    try(tablist_range_from_parsed_range(session_tablist(p->s), &p->rp, &r));

    TabNode* current_node = NULL;
    tryjmp(err,Clean, tablist_current_node(f, &current_node));
    size_t prev_offset = 0;

    foreach__ (TabNode,_tablist_tabs_(f), it)  {
        prev_offset += len__(nodes);
        arlfn(TabNodePtr,reset)(nodes);
        tryjmp(err,Clean, screen__(out, "    .\n"));
        tryjmp(err,Clean, tablist_to_node_list(it, nodes, out));

        const TabNodePtr* nodes_offset = arlfn(TabNodePtr,begin)(nodes);
        foreach__(TabNodePtr,nodes,n) {
            const size_t ix = prev_offset + n - nodes_offset;
            if (ix < r.beg) continue;
            if (r.end <= ix) goto Clean;
            if (*n == current_node) tryjmp(err,Clean, screen__(out, "[*] "));
            else if (tab_node_is_current_in_tab(*n)) tryjmp(err,Clean, screen__(out, "[ ] "));
            else tryjmp(err,Clean, screen__(out, "    "));
            tryjmp(err,Clean, cmd_out_std_append_ui_as_base36(out, ix));
            tryjmp(err,Clean, screen__(out, " "));
            str_reset(&buf);
            tryjmp(err,Clean, tab_node_to_bookmark_description(*n, &buf));
            tryjmp(err,Clean, screen_ln__(out, buf));
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


Err
tablist_current_doc(TabList f[_1_], HtmlDocPtr* out[_1_]) {
    TabNode* t;
    try( tablist_current_tab(f, &t));
    if (!t) { 
        *out = NULL;
        return Ok;
    }

    return tab_node_current_doc(t, out);
}

Err
tablist_set_currents_last_child_as_current(TabList tl[_1_]) {
    TabNode* current_tab;
    try( tablist_current_tab(tl, &current_tab));
    TabNode* cn;
    try( tab_node_current_node(current_tab, &cn));
    return tab_node_set_last_as_current(cn);
}


Err
tablist_range_from_parsed_range(TabList tl[_1_], RangeParse p[_1_], Range r[_1_]) {
    *r = (Range){0};
    size_t range_max = tablist_node_count(tl);
    switch (p->beg.tag) {
        case range_addr_curr_tag:
            try(tablist_current_node_ix(tl, &r->beg));
            break;
        case range_addr_none_tag:
        case range_addr_prev_tag: 
        case range_addr_search_tag:
            return "invalid range.beg address for tablist";
        case range_addr_beg_tag:
             try(set__(&r->beg, p->beg.delta));
            break;
        case range_addr_end_tag: r->beg = range_max + p->beg.delta;
            break;
        case range_addr_num_tag: r->beg = p->beg.n + p->beg.delta;
            break;
        default: return "error: invalid RangeParse beg tag";
    }
    switch (p->end.tag) {
        case range_addr_curr_tag:
            try(tablist_current_node_ix(tl, &r->beg));
            break;
        case range_addr_prev_tag: 
        case range_addr_search_tag:
             return "invalid range.end address for tablist";
        case range_addr_beg_tag: r->end = 1 + p->end.delta;
            break;
        case range_addr_none_tag: r->end = 1 + r->beg;
            break;
        case range_addr_end_tag: r->end = 1 + range_max + p->end.delta;
            break;
        case range_addr_num_tag: r->end = 1 + p->end.n + p->end.delta;
            break;
        default: return "error: invalid RangeParse end tag";
    }
    return Ok;
}


size_t
tablist_node_count(TabList tl[_1_]) {
    size_t count = 0;
    foreach__(TabNode,_tablist_tabs_(tl), it) {
        count += tab_node_count(it);
    }
    return count;
}

