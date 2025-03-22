#include "draw.h" 
#include "debug.h"
#include "bookmark.h"


Err draw_bookmark_rec(lxb_dom_node_t* node) {
    if (node) {
        switch(node->type) {
            //case LXB_DOM_NODE_TYPE_ELEMENT: return draw_rec_tag(node);
            //case LXB_DOM_NODE_TYPE_TEXT: return draw_text(node);
            //TODO: do not ignore these types?
            case LXB_DOM_NODE_TYPE_DOCUMENT: 
            case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case LXB_DOM_NODE_TYPE_COMMENT:
                //return draw_list(node->first_child, node->last_child, NULL);
                puts("g"); return Ok;
            default: {
                 //TODO: log this or do anythong else?
                //if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                //    log_warn__("lexbor node type greater than last entry: %lx\n", node->type);
                //else log_warn__("Ignored Node Type: %s\n", _dbg_node_types_[node->type]);
                return Ok;
            }
        }
    }
    return Ok;
}

Err cmd_bookmarks(Session session[static 1], const char* url) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(BufOf(char)) list = (ArlOf(BufOf(char))){0};
    lxb_dom_node_t* body;
    try(bookmark_sections_body(htmldoc, &body));
    Err err = Ok;
    if (!*url) {
        err = bookmark_sections(body, &list);
        if (!err) {
            BufOf(char)* it = arlfn(BufOf(char), begin)(&list);
            for (; it != arlfn(BufOf(char), end)(&list); ++it) {
                try( session_write_std(session, items__(it), len__(it)));
                try( session_write_std_lit__(session, "\n"));
            }
        }
        arlfn(BufOf(char),clean)(&list);
    } else {
        lxb_dom_node_t* section;
        try( bookmark_section_get(body, url, &section));

        if (section) {
            const char* data;
            size_t len;
            try( lexbor_node_get_text(section->first_child, &data, &len));
            if (len) {
                try(session_write_std(session, (char*)data, len));
                try(session_write_std_lit__(session, "\n"));
            }
        } else err = "invalid section in bookmark";
    }
    return err;
}
