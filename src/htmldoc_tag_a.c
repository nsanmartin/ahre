#include <assert.h>

#include "src/constants.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"


static inline  Err
_append_unsigned_to_bufof_char_base36(uintmax_t ui, BufOf(char)* b) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
    if (!buffn(char, append)(b, numbf, len)) return "error appending unsigned to bufof char";
    return Ok;
}

static bool _node_has_href(lxb_dom_node_t* node) {
    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(node, "href", &data, &data_len);
    return data && data_len;
}

bool browse_ctx_lazy_str_ends_two_newlines(BufOf(char)* lstr) {
    if (lstr->len < 2) return false;
    return buffn(char, end)(lstr)[-1] == '\n' && buffn(char, end)(lstr)[-2] == '\n';
}

Err browse_tag_a(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    /* https://html.spec.whatwg.org/multipage/links.html#attr-hyperlink-href
     * The href attribute on a and area elements is not required; when those
     * elements do not have href attributes they do not create hyperlinks. */
    bool is_hyperlink = _node_has_href(node);
    if (is_hyperlink) {

        HtmlDoc* d = browse_ctx_htmldoc(ctx);
        ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);
        const size_t anchor_num = anchors->len;
        if (!arlfn(LxbNodePtr,append)(anchors, &node)) 
            return "error: lip set";

        try( browse_ctx_exc_code_push(ctx, esc_code_blue));
        try( browse_ctx_lazy_str_append(ctx, EscCodeBlue, sizeof (EscCodeBlue)-1));
        try( browse_ctx_lazy_str_append(ctx, ANCHOR_OPEN_STR, sizeof (ANCHOR_OPEN_STR)-1));
        try(_append_unsigned_to_bufof_char_base36(anchor_num, &ctx->lazy_str));
        try( browse_ctx_lazy_str_append(ctx, ELEM_ID_SEP, sizeof(ELEM_ID_SEP)-1));
    }

    *browse_ctx_dirty(ctx) = false;
    try( browse_list(node->first_child, node->last_child, cb, ctx));

    try( browse_ctx_esc_code_pop(ctx));
    /* If not dirty, node's childs didn't write anything so
     * there's nothig to close.
     */
    if(!*browse_ctx_dirty(ctx)) {
        buffn(char, reset)(&ctx->lazy_str);
    } else if (is_hyperlink) {

        BufOf(char)* lstr = browse_ctx_lazy_str(ctx);
        while(browse_ctx_lazy_str_ends_two_newlines(lstr)) {
            --lstr->len;
        }

        try ( serialize_lit_str(ANCHOR_CLOSE_STR, cb, ctx));
        try ( serialize_literal_color_str(EscCodeReset, cb, ctx));
        try ( browse_ctx_lazy_str_serialize(ctx, cb));
    }
   
    return Ok;
}


