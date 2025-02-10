#include <assert.h>

#include "src/browse.h"
#include "src/constants.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"


//static inline  Err
//_append_unsigned_to_bufof_char_base36(uintmax_t ui, BufOf(char)* b) {
//    char numbf[3 * sizeof ui] = {0};
//    size_t len = 0;
//    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
//    if (!buffn(char, append)(b, numbf, len)) return "error appending unsigned to bufof char";
//    return Ok;
//}

static bool _node_has_href(lxb_dom_node_t* node) {
    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(node, "href", &data, &data_len);
    return data && data_len;
}

bool browse_ctx_buf_ends_two_newlines(BufOf(char)* lstr) {
    if (lstr->len < 2) return false;
    return buffn(char, end)(lstr)[-1] == '\n' && buffn(char, end)(lstr)[-2] == '\n';
}

Err browse_tag_a(lxb_dom_node_t* node, BrowseCtx ctx[static 1]) {
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

        try( browse_ctx_buf_append_color_(ctx, esc_code_blue));
        try( browse_ctx_buf_append_lit__(ctx, ANCHOR_OPEN_STR));
        try( browse_ctx_buf_append_ui_base36_(ctx, anchor_num));
        try( browse_ctx_buf_append_lit__(ctx, ELEM_ID_SEP));

        try( browse_list_inline(node->first_child, node->last_child, ctx));

        try( browse_ctx_buf_append_lit__(ctx, ANCHOR_CLOSE_STR));
        try( browse_ctx_reset_color(ctx));
    } else try( browse_list_inline(node->first_child, node->last_child, ctx));//TODO: do this?
   
    return Ok;
}


