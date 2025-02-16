#include <assert.h>

#include "src/browse.h"
#include "src/constants.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"


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

size_t _strview_trim_left_count_newlines_(StrView s[static 1]);
size_t _strview_trim_right_count_newlines_(StrView s[static 1]);

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

        if (browse_ctx_color(ctx)) try( browse_ctx_esc_code_push(ctx, esc_code_blue));

        BufOf(char) buf = browse_ctx_buf_get_reset(ctx);
        try( browse_list(node->first_child, node->last_child, ctx));
        browse_ctx_swap_buf(ctx, &buf);

        StrView content = strview(buf.items, buf.len);
        bool left_newlines = _strview_trim_left_count_newlines_(&content);
        bool right_newlines = _strview_trim_right_count_newlines_(&content);
        Err err;
        if (left_newlines) if ((err=browse_ctx_buf_append_lit__(ctx, "\n"))) {
            buffn(char, clean)(&buf);
            return err;
        }
        if ((err=browse_ctx_buf_append_color_esc_code(ctx, esc_code_blue))
            || (err=browse_ctx_buf_append_lit__(ctx, ANCHOR_OPEN_STR))
            || (err=browse_ctx_buf_append_ui_base36_(ctx, anchor_num))
            || (err=browse_ctx_buf_append_lit__(ctx, ELEM_ID_SEP))
        ) {
            buffn(char, clean)(&buf);
            return err;
        }

        if (content.len) try( browse_ctx_buf_append(ctx, (char*)content.s, content.len));
        buffn(char, clean)(&buf);

        try( browse_ctx_buf_append_lit__(ctx, ANCHOR_CLOSE_STR));
        try( browse_ctx_reset_color(ctx));
        if (right_newlines) try( browse_ctx_buf_append_lit__(ctx, "\n"));



    } else try( browse_list(node->first_child, node->last_child, ctx));//TODO: do this?
   
    return Ok;
}


