#include "tab-node.h"
#include "session.h"

Err htmldoc_init_fetch_draw(
    HtmlDoc     d[static 1],
    const char* urlstr,
    Url*        url,
    UrlClient   url_client[static 1],
    HttpMethod  method,
    Session     s[static 1]
);

Err tab_node_init(
    TabNode     n[static 1],
    TabNode*    parent,
    const char* urlstr,
    Url*        url,
    UrlClient   url_client[static 1],
    HttpMethod  method,
    Session     s[static 1]
) {
    try(_tab_node_init_base_(n, parent));
    Err err = htmldoc_init_fetch_draw(tab_node_doc(n), urlstr, url, url_client, method, s);
    if (err) {
        /* we don't want tab_node_cleanup to free node_doc again */
        *tab_node_doc(n) = (HtmlDoc){0};
        tab_node_cleanup(n);
        return err;
    }

    n->current_ix = n->childs->len;
    return Ok;
}


Err tab_node_tree_append_ahref(
    TabNode   t[static 1],
    size_t    linknum,
    UrlClient url_client[static 1],
    Session   s[static 1]
)
{
    TabNode* n;
    try(  tab_node_current_node(t, &n));
    if (!n) return "error: current node not found";
    HtmlDoc* d = &n->doc;
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, linknum);
    if (!a) return "link number invalid";

    /* CURLU* curlu = url_cu(htmldoc_url(d)); */
    /* //TODO: dup in htmldoc ctor */
    /* try( lexcurl_dup_curl_from_node_and_attr(*a, "href", 4, &curlu)); */
    Str url = (Str){0};
    try (lexbor_append_null_terminated_attr(*a, "href", 4, &url));

    TabNode newnode;
    Err err;
    try_or_jump(
        err,
        Clean_Url_Str,
        tab_node_init(&newnode, n, url.items, htmldoc_url(d), url_client, http_get, s)
    );
    /* if((err=tab_node_init(&newnode, n, NULL, &url, htmldoc_url(d), url_client, http_get, s))) { return err; }; */
    try_or_jump(
        err,
        Failure_New_Node_Cleanup,
        tab_node_append_move_child(n, &newnode)
    );
    /* if ((err=tab_node_append_move_child(n, &newnode))) { tab_node_cleanup(&newnode); return err; } */
    goto Clean_Url_Str;
Failure_New_Node_Cleanup:
    tab_node_cleanup(&newnode);
Clean_Url_Str:
    str_clean(&url);
    return Ok;
}

Err tab_node_tree_append_submit(
    TabNode t[static 1],
    size_t ix,
    UrlClient url_client[static 1],
    Session s[static 1]
) {
    TabNode* n;
    try(  tab_node_current_node(t, &n));
    if (!n) return "error: current node not found";

    HtmlDoc* d;
    try(  tab_node_current_doc(t, &d));
    if (!d) return "error: current doc not found";

    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(d);
    CURLU* curlu = curl_url_dup(url_cu(htmldoc_url(d)));
    if (!curlu) return "error: memory failure (curl_url_dup)";
    Url u = {.cu=curlu};

    Err e = Ok;
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(inputs, ix);
    if (!nodeptr) {
        e = "link number invalid";
        goto Clean_Url;
    }
    if (!_lexbor_attr_has_value(*nodeptr, "type", "submit")) {
        e = "warn: not submit input";
        goto Clean_Url;
    }

    lxb_dom_node_t* form = _find_parent_form(*nodeptr);
    if (form) {
        HttpMethod method;
        try_or_jump(e, Clean_Url, mk_submit_url(url_client, form, &curlu, &method));

        TabNode newnode;
        try_or_jump(e, Clean_Url, tab_node_init(&newnode, n, NULL, &u, url_client, method, s));
        try_or_jump(e, Clean_Url, tab_node_append_move_child(n, &newnode));
    } else { return "expected form, not found"; }

Clean_Url:
    url_cleanup(&u);

    return Ok;
}


void arl_of_tab_node_clean(ArlOf(TabNode)* f) {
    //TODO: fix #include TClean <- ...

    for (TabNode* it = arlfn(TabNode, begin)(f); it != arlfn(TabNode,end)(f); ++it) {
        tab_node_cleanup(it);
    }
    std_free((void*)f->items);
    *f = (ArlOf(TabNode)){0};
}


void tab_node_cleanup(TabNode n[static 1]) {
    htmldoc_cleanup(&n->doc);
    //arlfn(TabNode, clean)(n->childs);
    arl_of_tab_node_clean(n->childs);
    std_free(n->childs);
}

void _set_parent_to_all_(TabNode parent[static 1], ArlOf(TabNode) childs[static 1]) {
    for (TabNode* it = arlfn(TabNode,begin)(childs); it != arlfn(TabNode,end)(childs); ++it) {
        it->parent = parent;
    }
}

void _fix_childs_parents_(ArlOf(TabNode)* list) {
    for (TabNode* it = arlfn(TabNode,begin)(list); it != arlfn(TabNode,end)(list); ++it) {
        _set_parent_to_all_(it, tab_node_childs(it));
    }
}

TabNode* arl_of_tab_node_append(ArlOf(TabNode)* list, TabNode tn[static 1]) {
    TabNode* res = NULL;
    TabNode* mem_location = list->items;
    res = arlfn(TabNode, append)(list, tn);
    if (!res) return res;
    if (mem_location && mem_location != list->items) { _fix_childs_parents_(list); }
    return res;
}

Err session_tab_node_print(
    Session* s,
    TabNode n[static 1],
    size_t ix,
    ArlOf(size_t) stack[static 1],
    TabNode* current_node
) {
    if (!arlfn(size_t, append)(stack, &ix)) return "error: arl append failure";
    HtmlDoc* d = &n->doc;
    LxbNodePtr title = *htmldoc_title(d);

    for(size_t* it = arlfn(size_t, begin)(stack); it != arlfn(size_t, end)(stack); ++it) {
        if (it == arlfn(size_t, begin)(stack)) {
            if (n == current_node) {
                try( session_write_msg_lit__(s, "[+] "));
                try( session_write_unsigned_msg(s, *it));
                try( session_write_msg_lit__(s, "."));
            } else if (tab_node_is_current_in_tab(n)) {
                try( session_write_msg_lit__(s, "[ ] "));
                try( session_write_unsigned_msg(s, *it));
                try( session_write_msg_lit__(s, "."));
            } else {
                try( session_write_msg_lit__(s, "    "));
                try( session_write_unsigned_msg(s, *it));
                try( session_write_msg_lit__(s, "."));
            } } else { 
                try( session_write_unsigned_msg(s, *it));
                try( session_write_msg_lit__(s, "."));
            }
    }
    try( session_write_msg_lit__(s, " "));
    if (title) {
            Str title_text = (Str){0};
            Err e = lexbor_get_title_text_line(title, &title_text);
            ok_then(e, session_write_msg(s, title_text.items, title_text.len));
            ok_then(e, session_write_msg_lit__(s, "\n"));
            str_clean(&title_text);
    } else {
        char* buf;
        Err e = url_cstr_malloc(htmldoc_url(d), &buf);
        if (e) {
            try( session_write_msg_lit__(s, "error: "));
            try( session_write_msg(s, (char*)e, strlen(e)));
            try( session_write_msg_lit__(s, "\n"));
        } else {
            Err e = session_write_msg(s, buf, strlen(buf));
            ok_then(e, session_write_msg_lit__(s, "\n"));
            curl_free(buf);
            if (e) return e;
        }
    }

    TabNode* it = arlfn(TabNode, begin)(n->childs);
    const TabNode* beg = it;
    const TabNode* end = arlfn(TabNode, end)(n->childs);
    for (; it != end; ++it) {
        size_t subix = it-beg;
        try( session_tab_node_print(s, it, subix, stack, current_node));
    }
    if (!stack->len) return "error: arl pop failure";
    else --stack->len;
    return Ok;
}
