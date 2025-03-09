#include <assert.h>

#include "src/draw.h"
#include "src/constants.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"


static bool _node_has_href(lxb_dom_node_t* node) {
    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(node, "href", &data, &data_len);
    return data && data_len;
}

bool draw_ctx_buf_ends_two_newlines(BufOf(char)* lstr) {
    if (lstr->len < 2) return false;
    return buffn(char, end)(lstr)[-1] == '\n' && buffn(char, end)(lstr)[-2] == '\n';
}

size_t _strview_trim_left_count_newlines_(StrView s[static 1]);
size_t _strview_trim_right_count_newlines_(StrView s[static 1]);

Err draw_tag_a(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    /* https://html.spec.whatwg.org/multipage/links.html#attr-hyperlink-href
     * The href attribute on a and area elements is not required; when those
     * elements do not have href attributes they do not create hyperlinks. */
    bool is_hyperlink = _node_has_href(node);
    if (is_hyperlink) {

        HtmlDoc* d = draw_ctx_htmldoc(ctx);
        ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);
        const size_t anchor_num = anchors->len;
        if (!arlfn(LxbNodePtr,append)(anchors, &node)) 
            return "error: lip set";

        if (draw_ctx_color(ctx)) try( draw_ctx_esc_code_push(ctx, esc_code_blue));

        BufOf(char) buf = draw_ctx_buf_get_reset(ctx);
        try( draw_list(node->first_child, node->last_child, ctx));
        draw_ctx_swap_buf(ctx, &buf);

        StrView content = strview_from_mem(buf.items, buf.len);
        bool left_newlines = _strview_trim_left_count_newlines_(&content);
        bool right_newlines = _strview_trim_right_count_newlines_(&content);
        Err err;
        if (left_newlines) if ((err=draw_ctx_buf_append_lit__(ctx, "\n"))) {
            buffn(char, clean)(&buf);
            return err;
        }
        if ((err=draw_ctx_buf_append_color_esc_code(ctx, esc_code_blue))
            || (err=draw_ctx_buf_append_lit__(ctx, ANCHOR_OPEN_STR))
            || (err=draw_ctx_buf_append_ui_base36_(ctx, anchor_num))
            || (err=draw_ctx_buf_append_lit__(ctx, ANCHOR_CLOSE_STR))
        ) {
            buffn(char, clean)(&buf);
            return err;
        }

        if (content.len) try( draw_ctx_buf_append(ctx, (char*)content.items, content.len));
        buffn(char, clean)(&buf);

        try( draw_ctx_reset_color(ctx));
        if (right_newlines) try( draw_ctx_buf_append_lit__(ctx, "\n"));



    } else try( draw_list(node->first_child, node->last_child, ctx));//TODO: do this?
   
    return Ok;
}


