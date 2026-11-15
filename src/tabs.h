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

#ifndef AHRE_TABS_H__
#define AHRE_TABS_H__


#include "tab-node.h"

void arl_of_tab_node_clean(ArlOf(TabNode)* t);

typedef struct {
    ArlOf(TabNode) tabs;
    size_t current_tab;
} TabList;


Err tablist_current_doc(TabList f[_1_], HtmlDocPtr* out[_1_]);


/* getter */

static inline ArlOf(TabNode)* _tablist_tabs_(TabList f[_1_]) {
    return &f->tabs;
}

static inline size_t _tablist_tab_count_(TabList f[_1_]) {
    return _tablist_tabs_(f)->len;
}

static inline bool tablist_is_empty(TabList f[_1_]) {
    return _tablist_tab_count_(f) == 0;
}

static inline size_t* _tablist_current_tab_ix_(TabList f[_1_]) { return &f->current_tab; }
static inline Err
tablist_current_tab(TabList f[_1_], TabNode* out[_1_]) {
    TabNode* current_tab = arlfn(TabNode, at)(&f->tabs, f->current_tab);
    if (!current_tab) return "no tabs in session";
    if (tablist_is_empty(f)) return "error: current tab has no nodes";
    *out = current_tab;
    return Ok;
}

static inline
Err tablist_current_node(TabList f[_1_], TabNode* out[_1_]) {
    TabNode* root;
    try( tablist_current_tab(f, &root));
    if (!root) { 
        *out = NULL;
        return Ok;
    }

    return tab_node_current_node(root, out);
}


static inline
Err tablist_current_node_ix(TabList f[_1_], size_t out[_1_]) {
    if (!f->tabs.len) return "no tabs in session";
    if (f->tabs.len <= f->current_tab) fail_e("bad current tab");

    *out      = 0;
    size_t ix = 0;

    while (ix < f->current_tab) {
        *out += tab_node_count(arlfn(TabNode,at)(_tablist_tabs_(f),ix));
    }

    TabNode* node = arlfn(TabNode,at)(_tablist_tabs_(f),f->current_tab);

    return tab_node_current_node_ix(node, out);
}

static inline Err
tablist_append_move_tree(TabList f[_1_], TabNode t[_1_]) {
    if (!arl_of_tab_node_append(&f->tabs, t))  return "error: arl append failure";
    return Ok;
}



/* dtor */
static inline void tablist_cleanup(TabList f[_1_]) {
    arlfn(TabNode, clean)(&f->tabs);
}

Err tablist_info_tree(TabList f[_1_], CmdOut* out);
Err tablist_info_titles(TabList f[_1_], CmdOut* out);

static inline
Err tablist_back(TabList tl[_1_]) {
    TabNode* cn;
    try( tablist_current_node(tl, &cn));
    if (!cn) return "can't go back with no current node";
    if (!cn->parent) return "can't go back if tab's root";
    tab_node_set_as_current(cn->parent);
    return Ok;
}

Err tablist_move_to_node(TabList tl[_1_], const char* line);


static inline
Err tablist_move_to_path(TabList tl[_1_], const char* line) {
    size_t ix;
    try( parse_size_t_or_throw(&line, &ix, 10));
    TabNode* tn = arlfn(TabNode,at)(_tablist_tabs_(tl), ix);
    if (!tn) return "invalid tab";

    line = cstr_skip_space(line);
    if (!*line) {
        tl->current_tab = ix;
        return Ok;
    }
    if (*line != '.') return "invalid tab path";

    TabNode* search;
    try( tab_node_find_node(tn, line + 1, &search));
    try( tab_node_set_as_current(search));
    *_tablist_current_tab_ix_(tl) = ix;

    return Ok;
}

Err tablist_set_currents_last_child_as_current(TabList tl[_1_]);
size_t tablist_node_count(TabList tl[_1_]);
Err tablist_range_from_parsed_range(TabList tl[_1_], RangeParse p[_1_], Range r[_1_]);
Err tablist_info_titles_range(CmdParams p[_1_]);
Err tablist_move_to_node_ix(TabList tl[_1_], size_t ix);
#endif
