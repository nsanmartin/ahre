#include "src/debug.h"


static Err _dbg_print_form_info_rec_(lxb_dom_node_t* node, int indent) {
    if (!node) return Ok;
    if (node->local_name == LXB_TAG_INPUT ) {

        const lxb_char_t* name;
        size_t namelen;
        lexbor_find_attr_value(node, "name", &name, &namelen);
        if (!name || namelen == 0 || strcmp((char*)name, "password") == 0) return Ok;

        const lxb_char_t* value;
        size_t valuelen;
        lexbor_find_attr_value(node, "value", &value, &valuelen);

        if (!value || valuelen == 0)
            log_debug__(
                "%*c" "%p->%p\t" "%.*s" "\t\\0\n",
                indent, ' ', (void*)node->parent, (void*)node ,namelen, name
            );
        else
            log_debug__(
                "%*c" "%p->%p\t" "%.*s" "\t" "%.*s\n",
                indent, ' ', (void*)node->parent, (void*)node ,namelen, name, valuelen, value
            );
        return Ok;
    } 

    for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
        try( _dbg_print_form_info_rec_(it, indent+1));
        if (it == node->last_child) { break; }
    }
    return Ok;
}

Err _dbg_print_form_info_(lxb_dom_node_t* node) {
    if (node->local_name != LXB_TAG_FORM) { puts("dgb err, expecting a FORM"); return Ok; } 
    return _dbg_print_form_info_rec_(node, 0);
}


Err dbg_print_form(Session s[static 1], const char* line) {
    try_debug_build_only();

    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(!current_tab) return "error: no current tab";

    TabNode* n;
    try(  tab_node_current_node(current_tab, &n));
    if (!n) return "error: current node not found";
    HtmlDoc* d = &n->doc;
    ArlOf(LxbNodePtr)* forms = htmldoc_forms(d);

    LxbNodePtr* formp = arlfn(LxbNodePtr, at)(forms, linknum);
    if (!formp) return "link number invalid";

    return _dbg_print_form_info_(*formp);
}

