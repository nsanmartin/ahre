#include <assert.h>

#include "src/constants.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"

static Err
_browse_childs(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (!node) return Ok;
    for ( lxb_dom_node_t* it = node->first_child
        ; it
        ; it = it->next
    ) {
        assert(it);
        if (it->type == LXB_DOM_NODE_TYPE_TEXT) {
            //DUP CODE
            size_t len = lxb_dom_interface_text(it)->char_data.data.length;
            const char* data = (const char*)lxb_dom_interface_text(it)->char_data.data.data;
            if (mem_is_all_space((char*)data, len)) { continue; }

            if (ctx->lazy_str.len) {
                lxb_char_t* lazy_str = (lxb_char_t*)ctx->lazy_str.items;
                size_t lazy_str_len = ctx->lazy_str.len;
                while (len > 1 && isspace(lazy_str[0]) && isspace(lazy_str[1])) {
                    ++lazy_str; -- lazy_str_len;
                }
                if(cb(lazy_str, lazy_str_len, ctx)) return "error serializing nl";
                buffn(char, reset)(&ctx->lazy_str);
            }

            while((data = cstr_skip_space((const char*)data))) {
                if (!*data) break;
                const char* end = cstr_next_space((const char*)data);
                if (data == end) break;
                if (cb((lxb_char_t*)data, end-data, ctx)) return "error serializing html text elem";
                if (cb((lxb_char_t*)" ", 1, ctx)) return "error serializing space";
                data = end;
            }
        
        } else { 
            _browse_childs(it, cb, ctx);
        }

    }
    return Ok;
}


static inline  Err
_append_unsigned_to_bufof_char_base36(uintmax_t ui, BufOf(char)* b) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
    if (!buffn(char, append)(b, numbf, len)) return "error appending unsigned to bufof char";
    return Ok;
}

bool _node_has_href(lxb_dom_node_t* node) {
    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(node, "href", &data, &data_len);
    return data && data_len;
}


Err browse_tag_a(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    /* https://html.spec.whatwg.org/multipage/links.html#attr-hyperlink-href
     * The href attribute on a and area elements is not required; when those
     * elements do not have href attributes they do not create hyperlinks. */
    bool is_hyperlink = _node_has_href(node);
    if (is_hyperlink) {
        if (!buffn(char, append)(&ctx->lazy_str, EscCodeBlue, sizeof(EscCodeBlue)-1))
            return "error: failed to append to bufof";


        HtmlDoc* d = browse_ctx_htmldoc(ctx);
        ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);
        const size_t anchor_num = anchors->len;
        if (!arlfn(LxbNodePtr,append)(anchors, &node)) 
            return "error: lip set";

        if (!buffn(char, append)(&ctx->lazy_str, EscCodeBlue, sizeof(EscCodeBlue)-1))
            return "error: failed to append to bufof";
        if (!buffn(char, append)(&ctx->lazy_str, ANCHOR_OPEN_STR, sizeof(ANCHOR_CLOSE_STR)-1))
            return "error: failed to append to bufof";
        try(_append_unsigned_to_bufof_char_base36(anchor_num, &ctx->lazy_str));
        if (!buffn(char, append)(&ctx->lazy_str, " ", 1)) return "error: failed to append to bufof";
    }

    try ( _browse_childs(node, cb, ctx));

    /* If lazy string is not empty, node's childs didn't write anything so
     * there's nothig to close.
     */
    if (ctx->lazy_str.len) {
        buffn(char, reset)(&ctx->lazy_str);
    } else if (is_hyperlink) {
        try ( serialize_lit_str(ANCHOR_CLOSE_STR, cb, ctx));
        try ( serialize_literal_color_str(EscCodeReset, cb, ctx));
    }
   
    return Ok;
}


