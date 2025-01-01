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
                if(cb((lxb_char_t*)ctx->lazy_str.items, ctx->lazy_str.len, ctx))
                    return "error serializing nl";
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
_append_unsigned_to_bufof_char(uintmax_t ui, BufOf(char)* b) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
    if (!buffn(char, append)(b, numbf, len)) return "error appending unsigned to bufof char";
    return Ok;
}



Err parse_append_ahref(BrowseCtx ctx[static 1], const char* url, size_t len) {
    if (!url) return "error: NULL url";

    ArlOf(Ahref)* ahrefs = htmldoc_ahrefs(browse_ctx_htmldoc(ctx));
    if (!buffn(char, append)(&ctx->lazy_str, ANCHOR_OPEN_STR, sizeof IMAGE_OPEN_STR))
        return "error: failed to append to bufof";
    try(_append_unsigned_to_bufof_char(ahrefs->len, &ctx->lazy_str));
    if (!buffn(char, append)(&ctx->lazy_str, " ", 1)) return "error: failed to append to bufof";

    Ahref a = (Ahref){0};
    if (ahref_init_alloc(&a, url, len, textbuf_len(browse_ctx_textbuf(ctx))))
        return "error: intialiazing Ahref";
    if (!arlfn(Ahref,append)(ahrefs, &a)) {
        free((char*)a.url);
        return "error: lip set";
    }
    return Ok;
}


Err parse_href_attrs(lxb_dom_node_t* node, BrowseCtx ctx[static 1]) {
    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(node, "href", &data, &data_len);
    if (data != NULL) {
        Err err = parse_append_ahref(ctx, (const char*)data, data_len);
        if (err) return err;
    } else
        puts("warn log: expecting 'href' but not found");
    return Ok;
}

//TODO: move instance of Ahref here as well as appending it
Err browse_tag_a(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (!buffn(char, append)(&ctx->lazy_str, EscCodeBlue, sizeof EscCodeBlue)) return "error: failed to append to bufof";
    try ( parse_href_attrs(node, ctx));
    try ( _browse_childs(node, cb, ctx));
    try ( serialize_lit_str(ANCHOR_CLOSE_STR, cb, ctx));
    try ( serialize_literal_color_str(EscCodeBlue, cb, ctx));
    try ( serialize_literal_color_str(EscCodeReset, cb, ctx));
    return Ok;
}


