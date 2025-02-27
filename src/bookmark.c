#include "src/browse.h" 
#include "src/debug.h"
#include "src/bookmark.h"


Err browse_bookmark_rec(lxb_dom_node_t* node) {
    if (node) {
        switch(node->type) {
            //case LXB_DOM_NODE_TYPE_ELEMENT: return browse_rec_tag(node);
            //case LXB_DOM_NODE_TYPE_TEXT: return browse_text(node);
            //TODO: do not ignore these types?
            case LXB_DOM_NODE_TYPE_DOCUMENT: 
            case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case LXB_DOM_NODE_TYPE_COMMENT:
                //return browse_list(node->first_child, node->last_child, NULL);
                puts("g"); return Ok;
            default: {
                if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                    log_warn__("lexbor node type greater than last entry: %lx\n", node->type);
                //else log_warn__("Ignored Node Type: %s\n", _dbg_node_types_[node->type]);
                return Ok;
            }
        }
    }
    return Ok;
}

