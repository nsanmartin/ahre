[1mdiff --git a/1 b/1[m
[1mdeleted file mode 100644[m
[1mindex 72e8d39..0000000[m
[1m--- a/1[m
[1m+++ /dev/null[m
[36m@@ -1,15 +0,0 @@[m
[31m-cc -g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition -I. -Ihashi/include -Iisocline/include -fsanitize=leak -fsanitize=address -fsanitize=undefined -fsanitize=null -fsanitize=bounds -fsanitize=pointer-overflow -I. -I/home/nico/usr/include -Iutests -o build/test_ed_write utests/test_ed_write.c build/error.o build/str.o build/ranges.o build/textbuf.o build/session.o [m
[31m-/usr/bin/ld: build/ranges.o: in function `parse_linenum_search_regex':[m
[31m-/home/nico/github/nsanmartin/ahre/src/ranges.c:69: undefined reference to `regex_maybe_find_next'[m
[31m-/usr/bin/ld: build/session.o: in function `htmldoc_fetch':[m
[31m-/home/nico/github/nsanmartin/ahre/./src/htmldoc.h:110: undefined reference to `curl_lexbor_fetch_document'[m
[31m-/usr/bin/ld: build/session.o: in function `session_create':[m
[31m-/home/nico/github/nsanmartin/ahre/src/session.c:22: undefined reference to `url_client_create'[m
[31m-/usr/bin/ld: /home/nico/github/nsanmartin/ahre/src/session.c:25: undefined reference to `htmldoc_create'[m
[31m-/usr/bin/ld: /home/nico/github/nsanmartin/ahre/src/session.c:31: undefined reference to `htmldoc_browse'[m
[31m-/usr/bin/ld: /home/nico/github/nsanmartin/ahre/src/session.c:44: undefined reference to `url_client_destroy'[m
[31m-/usr/bin/ld: build/session.o: in function `session_destroy':[m
[31m-/home/nico/github/nsanmartin/ahre/src/session.c:52: undefined reference to `htmldoc_destroy'[m
[31m-/usr/bin/ld: /home/nico/github/nsanmartin/ahre/src/session.c:53: undefined reference to `url_client_destroy'[m
[31m-collect2: error: ld returned 1 exit status[m
[31m-make: *** [Makefile:64: test_ed_write] Error 1[m
[1mdiff --git a/TODO b/TODO[m
[1mindex 97ffd04..67d67a2 100644[m
[1m--- a/TODO[m
[1m+++ b/TODO[m
[36m@@ -24,6 +24,8 @@[m
 [?] fix url construction on a href (it could be absolute or not)[m
 [ ] display form elements with <>[m
 [ ] fix last char in some files saved by write ed cmd[m
[32m+[m[32m[ ] set autocomplete with ahre commands[m
[32m+[m[32m[ ] bug: image number incalid: check base[m
 [m
 # DONE[m
 [*] Accept files apart from urls[m
[1mdiff --git a/src/htmldoc.c b/src/htmldoc.c[m
[1mindex a79cc5d..ad6bb1d 100644[m
[1m--- a/src/htmldoc.c[m
[1m+++ b/src/htmldoc.c[m
[36m@@ -289,7 +289,10 @@[m [mErr htmldoc_init(HtmlDoc d[static 1], const char* cstr_url) {[m
     }[m
 [m
     *d = (HtmlDoc){[m
[31m-        .lxbdoc=document, .url=url, .cache=(DocCache){.textbuf=(TextBuf){.current_line=1}}[m
[32m+[m[32m        .url=url,[m
[32m+[m[32m        .method=http_get,[m
[32m+[m[32m        .lxbdoc=document,[m
[32m+[m[32m        .cache=(DocCache){.textbuf=(TextBuf){.current_line=1}}[m
     };[m
     return Ok;[m
 }[m
[1mdiff --git a/src/htmldoc.h b/src/htmldoc.h[m
[1mindex 12d10d2..554c4f9 100644[m
[1m--- a/src/htmldoc.h[m
[1m+++ b/src/htmldoc.h[m
[36m@@ -29,9 +29,11 @@[m [mtypedef struct {[m
     ArlOf(LxbNodePtr) inputs;[m
 } DocCache;[m
 [m
[32m+[m[32mtypedef enum { http_get = 0, http_post = 1 } HttpMethod;[m
 [m
 typedef struct {[m
     Url url;[m
[32m+[m[32m    HttpMethod method;[m
     lxb_html_document_t* lxbdoc;[m
     DocCache cache;[m
 } HtmlDoc;[m
[36m@@ -61,6 +63,9 @@[m [mhtmldoc_cache(HtmlDoc d[static 1]) { return &d->cache; }[m
 static inline Url*[m
 htmldoc_url(HtmlDoc d[static 1]) { return &d->url; }[m
 [m
[32m+[m[32mstatic inline HttpMethod[m
[32m+[m[32mhtmldoc_method(HtmlDoc d[static 1]) { return d->method; }[m
[32m+[m
 [m
 /* ctors */[m
 Err htmldoc_init(HtmlDoc d[static 1], const char* url);[m
[1mdiff --git a/src/user-interface.c b/src/user-interface.c[m
[1mindex 48723db..840c1e5 100644[m
[1m--- a/src/user-interface.c[m
[1m+++ b/src/user-interface.c[m
[36m@@ -15,7 +15,7 @@[m
 #include "src/wrapper-lexbor-curl.h"[m
 [m
 //TODO: move this to wrapper-lexbor-curl.h[m
[31m-Err mk_submit_url (lxb_dom_node_t* form, CURLU* out[static 1]);[m
[32m+[m[32mErr mk_submit_url(lxb_dom_node_t* form, CURLU* out[static 1], HttpMethod http_method[static 1]);[m
 [m
 Err read_line_from_user(Session session[static 1]) {[m
     char* line = 0x0;[m
[36m@@ -131,7 +131,7 @@[m [mErr _cmd_submit_ix(Session session[static 1], size_t ix) {[m
     lxb_dom_node_t* form = _find_parent_form(*nodeptr);[m
     if (form) {[m
 [m
[31m-        if ((err = mk_submit_url(form, &curlu))) {[m
[32m+[m[32m        if ((err = mk_submit_url(form, &curlu, &htmldoc->method))) {[m
             curl_url_cleanup(curlu);[m
             return err;[m
         }[m
[36m@@ -318,7 +318,7 @@[m [mErr cmd_anchor_eval(Session session[static 1], const char* line) {[m
     //if (*line && *cstr_skip_space(line + 1)) return "error unexpected:..."; TODO: only check when necessary[m
     switch (*line) {[m
         case '\'': return cmd_anchor_print(session, (size_t)linknum); [m
[31m-        case '"': return cmd_anchor_print(session, (size_t)linknum); [m
[32m+[m[32m        case '?': return cmd_anchor_print(session, (size_t)linknum);[m[41m [m
         case '*': return cmd_anchor_asterisk(session, (size_t)linknum);[m
         default: return "?";[m
     }[m
[36m@@ -331,7 +331,7 @@[m [mErr cmd_input_eval(Session session[static 1], const char* line) {[m
     line = cstr_skip_space(line);[m
     //if (*line && *cstr_skip_space(line + 1)) return "error unexpected:..."; TODO ^[m
     switch (*line) {[m
[31m-        case '"': return cmd_input_print(session, linknum);[m
[32m+[m[32m        case '?': return cmd_input_print(session, linknum);[m
         case '*': return _cmd_submit_ix(session, linknum);[m
         case '=': return _cmd_input_ix(session, linknum, line + 1); [m
         default: return "?";[m
[36m@@ -344,7 +344,7 @@[m [mErr cmd_image_eval(Session session[static 1], const char* line) {[m
     try( parse_base36_or_throw(&line, &linknum));[m
     line = cstr_skip_space(line);[m
     switch (*line) {[m
[31m-        case '"': return cmd_image_print(session, linknum);[m
[32m+[m[32m        case '?': return cmd_image_print(session, linknum);[m
         default: return "?";[m
     }[m
     return Ok;[m
[1mdiff --git a/src/wrapper-lexbor-curl.c b/src/wrapper-lexbor-curl.c[m
[1mindex 6708be6..7ffbe6c 100644[m
[1m--- a/src/wrapper-lexbor-curl.c[m
[1m+++ b/src/wrapper-lexbor-curl.c[m
[36m@@ -19,6 +19,15 @@[m [mcurl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[stati[m
     }[m
 [m
     Url* url = htmldoc_url(htmldoc);[m
[32m+[m
[32m+[m[32m    CURLoption method = htmldoc_method(htmldoc) == http_post[m[41m [m
[32m+[m[32m               ? CURLOPT_POST[m
[32m+[m[32m               : CURLOPT_HTTPGET[m
[32m+[m[32m               ;[m
[32m+[m[32m    if (curl_easy_setopt(url_client->curl, method, url_cu(url))) {[m
[32m+[m[32m        return "error: curl failed to set method";[m
[32m+[m[32m    }[m
[32m+[m
     if (curl_easy_setopt(url_client->curl, CURLOPT_CURLU, url_cu(url))) {[m
         return "error: curl failed to set url";[m
     }[m
[36m@@ -106,8 +115,45 @@[m [mstatic Err _submit_url_set_action([m
     return Ok;[m
 }[m
 [m
[31m-Err mk_submit_url (lxb_dom_node_t* form, CURLU* out[static 1]) {[m
[32m+[m[32m//Err _get_form_method_(lxb_dom_node_t* form, HttpMethod out[static 1]) {[m
[32m+[m[32m//    const lxb_char_t* method;[m
[32m+[m[32m//    size_t len;[m
[32m+[m[32m//    lexbor_find_attr_value(form, "action", &method, &len);[m
[32m+[m[32m//    if (!method || !len || lexbor_str_eq("get", method, len)) {[m
[32m+[m[32m//        *out = http_get;[m
[32m+[m[32m//        return Ok;[m
[32m+[m[32m//    }[m
[32m+[m[32m//[m
[32m+[m[32m//    if (method && len && lexbor_str_eq("post", method, len)) {[m
[32m+[m[32m//        *out = http_post;[m
[32m+[m[32m//        return Ok;[m
[32m+[m[32m//    }[m
[32m+[m[32m//    return "error: unsuported method";[m
[32m+[m[32m//}[m
[32m+[m
[32m+[m[32mstatic Err mk_submit_get([m
[32m+[m[32m    lxb_dom_node_t* form, const lxb_char_t* action, size_t action_len, CURLU* out[static 1][m
[32m+[m[32m) {[m
     Err err;[m
[32m+[m[32m    BufOf(lxb_char_t)* buf = &(BufOf(lxb_char_t)){0};[m
[32m+[m[32m    if (action && action_len) {[m
[32m+[m[32m        if ((err = _submit_url_set_action(buf, action, action_len, *out))) {[m
[32m+[m[32m            buffn(lxb_char_t, clean)(buf);[m
[32m+[m[32m            return err;[m
[32m+[m[32m        }[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    if ((err=_make_submit_curlu_rec(form, buf, *out))) {[m
[32m+[m[32m        buffn(lxb_char_t, clean)(buf);[m
[32m+[m[32m        return err;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    buffn(lxb_char_t, clean)(buf);[m
[32m+[m[32m    return Ok;[m
[32m+[m[32m}[m
[32m+[m
[32m+[m[32mErr mk_submit_url (lxb_dom_node_t* form, CURLU* out[static 1], HttpMethod http_method[static 1]) {[m
[32m+[m[32m    //Err err;[m
 [m
     const lxb_char_t* action;[m
     size_t action_len;[m
[36m@@ -118,21 +164,8 @@[m [mErr mk_submit_url (lxb_dom_node_t* form, CURLU* out[static 1]) {[m
     lexbor_find_attr_value(form, "method", &method, &method_len);[m
 [m
     if (!method_len || lexbor_str_eq("get", method, method_len)) {[m
[31m-        BufOf(lxb_char_t)* buf = &(BufOf(lxb_char_t)){0};[m
[31m-        if (action && action_len) {[m
[31m-            if ((err = _submit_url_set_action(buf, action, action_len, *out))) {[m
[31m-                buffn(lxb_char_t, clean)(buf);[m
[31m-                return err;[m
[31m-            }[m
[31m-        }[m
[31m-[m
[31m-        if ((err=_make_submit_curlu_rec(form, buf, *out))) {[m
[31m-            buffn(lxb_char_t, clean)(buf);[m
[31m-            return err;[m
[31m-        }[m
[31m-[m
[31m-        buffn(lxb_char_t, clean)(buf);[m
[31m-        return Ok;[m
[32m+[m[32m        *http_method = http_get;[m
[32m+[m[32m        return mk_submit_get(form, action, action_len, out);[m
     }[m
     return "not a get method";[m
 }[m
