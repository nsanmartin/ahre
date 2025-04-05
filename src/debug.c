#include "debug.h"
#include "cmd.h"


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
                output_stdout__,
                "%*c" "%p->%p\t" "%.*s" "\t\\0\n",
                indent, ' ', (void*)node->parent, (void*)node ,namelen, name
            );
        else
            log_debug__(
                output_stdout__,
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


Err dbg_print_form(CmdParams p[static 1]) {
    try_debug_build_only();

    p->ln = cstr_skip_space(p->ln);
    size_t linknum;
    try( parse_size_t_or_throw(&p->ln, &linknum, 36));
    p->ln = cstr_skip_space(p->ln);
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(p->s), &current_tab));
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

/*
 * Traversal
 */
char* _dbg_get_node_type_name_(size_t node_type);
char* _dbg_get_tag_name_(size_t local_name);
Err dbg_traversal_rec(lxb_dom_node_t* node, Session ctx[static 1]);

Err dbg_traversal_text(lxb_dom_node_t* node,  Session ctx[static 1]) {
    const char* data;
    size_t len;
    try( lexbor_node_get_text(node, &data, &len));

    //if (draw_ctx_pre(ctx)) { return Ok; } else
    if (mem_skip_space_inplace(&data, &len)) {
        //.try( draw_mem_skipping_space(data, len, ctx));
        session_write_msg_lit__(ctx, "{TEXT BEG}<<<\n");
        session_write_msg(ctx, (char*)data, len);
        session_write_msg_lit__(ctx, "\n>>>{TEXT END}\n");
    } else {
        session_write_msg_lit__(ctx, "{TEXT} (whitespace)\n");
    }
    return Ok;
}

static inline Err dbg_traversal_list(lxb_dom_node_t* it, lxb_dom_node_t* last, Session ctx[static 1]) {
    for(; it ; it = it->next) {
        try( dbg_traversal_rec(it, ctx));
        if (it == last) break;
    }
    return Ok;
}


Err dbg_traversal_rec_tag(lxb_dom_node_t* node, Session ctx[static 1]) {
    char* name = _dbg_get_tag_name_(node->local_name);
    session_write_msg(ctx, name, strlen(name));
    session_write_msg_lit__(ctx, "\n");
    return dbg_traversal_list(node->first_child, node->last_child, ctx);
}

Err dbg_traversal_rec(lxb_dom_node_t* node, Session ctx[static 1]) {
    if (node) {
        switch(node->type) {
            case LXB_DOM_NODE_TYPE_ELEMENT: return dbg_traversal_rec_tag(node, ctx);
            case LXB_DOM_NODE_TYPE_TEXT: return dbg_traversal_text(node, ctx);
            case LXB_DOM_NODE_TYPE_DOCUMENT: 
            case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case LXB_DOM_NODE_TYPE_COMMENT:
                return dbg_traversal_list(node->first_child, node->last_child, ctx);
            default: {
                if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY) {
                    session_write_msg_lit__(ctx, "lexbor node type greater than last entry");
                    session_write_msg_lit__(ctx, "\n");
                }
                else {
                    char* name = _dbg_get_node_type_name_(node->type);
                    session_write_msg_lit__(ctx, "Ignored Node Type");
                    session_write_msg(ctx, name, strlen(name));
                    session_write_msg_lit__(ctx, "\n");
                }
                return Ok;
            }
        }
    }
    return Ok;
}

Err _cmd_doc_dbg_traversal(Session ctx[static 1], const char* fname) {
    HtmlDoc* d;
    try( session_current_doc(ctx, &d));
    fname = cstr_skip_space(fname);

    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(d);
    str_reset(session_msg(ctx));
    try( dbg_traversal_rec(lxb_dom_interface_node(lxbdoc), ctx));
    FILE* fp = fopen(fname, "w");
    if (!fp) return err_fmt("%s: could not open file: %s", __func__, fname); 
    fwrite(items__(session_msg(ctx)), 1, len__(session_msg(ctx)), fp);
    fclose(fp);
    return Ok;
}
/* Traversal */
