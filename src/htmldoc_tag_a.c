#include <assert.h>

#include "draw.h"
#include "constants.h"
#include "htmldoc.h"
#include "wrapper-lexbor.h"


Err _hypertext_id_open_(
    DrawCtx ctx[static 1],
    ImpureDrawProcedure visual_effect,
    StrViewProvider open_str_provider,
    const size_t* id_num_ptr,
    StrViewProvider sep_str_provider
);

Err _hypertext_id_close_(
    DrawCtx ctx[static 1],
    ImpureDrawProcedure visual_effect,
    StrViewProvider close_str_provider
);

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

static bool _prev_is_separable_(lxb_dom_node_t n[static 1]) {
    return n->prev  && 
        n->prev->local_name != LXB_TAG_LI
        ;
}

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

        DrawSubCtx sub = (DrawSubCtx){0};
        draw_ctx_swap_sub(ctx, &sub);

        if (draw_ctx_color(ctx)) try( draw_ctx_esc_code_push(ctx, esc_code_blue));
        try( draw_list(node->first_child, node->last_child, ctx));

        draw_ctx_swap_sub(ctx, &sub);

        draw_subctx_trim_left(&sub);
        draw_subctx_trim_right(&sub);

        Err err = Ok;
        if (_prev_is_separable_(node)) err = draw_ctx_buf_append_lit__(ctx, " ");
        else if (sub.left_newlines) err = draw_ctx_buf_append_lit__(ctx, "\n");
        ok_then(err, _hypertext_id_open_(
                ctx, draw_ctx_textmod_blue, anchor_open_str, &anchor_num,anchor_sep_str ));

        if (!err && sub.buf.len) ok_then(err, draw_ctx_append_subctx(ctx, &sub));
        draw_subctx_clean(&sub);
        ok_then(err, _hypertext_id_close_(ctx, draw_ctx_reset_color, anchor_close_str));
        if (sub.right_newlines) ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
        try(err);

    } else try( draw_list(node->first_child, node->last_child, ctx));//TODO: do this?
   
    return Ok;
}


