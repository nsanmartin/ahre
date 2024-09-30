#include "src/debug.h"
#include "src/generic.h"
#include "src/html-doc.h"



/*
 * Functions that append summary of the content for debugging purposes
 */
static Err textbuf_summary(HtmlDoc html_doc[static 1], BufOf(char)* buf) {

    buf_append_lit("HtmlDoc: ", buf);
    if (buf_append_hexp(html_doc, buf)) { return "could not append"; }
    buf_append_lit("\n", buf);

    if (html_doc->url) {
        buf_append_lit(" url: ", buf);
       if (buffn(char,append)(buf, (char*)html_doc->url, strlen(html_doc->url))) {
           return  "could not append";
       }
       buf_append_lit("\n", buf);

    } else {
       buf_append_lit(" none\n", buf);

    }
    if (html_doc->lxbdoc) {
        buf_append_lit(" lxbdoc: ", buf);
        if (buf_append_hexp(html_doc->lxbdoc, buf)) { return  "could not append"; }
        buf_append_lit("\n", buf);

    }
    return Ok;
}

Err url_client_summary(UrlClient url_client[static 1], BufOf(char)*buf) {
   buf_append_lit("UrlClient: ", buf);
   if (buf_append_hexp(url_client, buf)) { return  "could not append"; }
   buf_append_lit("\n", buf);

   if (url_client->curl) {
       buf_append_lit(" curl: ", buf);
       if (buf_append_hexp(url_client->curl, buf)) { return  "could not append"; }
       buf_append_lit("\n", buf);

       if (url_client->errbuf[0]) {
           size_t slen = strlen(url_client->errbuf);
           if (slen > CURL_ERROR_SIZE) { slen = CURL_ERROR_SIZE; }
           if (buffn(char,append)(buf, url_client->errbuf, slen)) {
               return  "could not append";
           }
           buf_append_lit("\n", buf);
       }
   } else {
       buf_append_lit( " not curl\n", buf);
   }
   return Ok;
}


Err dbg_session_summary(Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (session->url_client) {
        if(url_client_summary(session->url_client, buf)) { return "could not get url client summary"; }
    } else { buf_append_lit("Not url_client\n", buf); }

    if (session->html_doc) {
        textbuf_summary(session->html_doc, buf);
        doc_cache_buffer_summary(&session->html_doc->cache, buf);
    } else { buf_append_lit("Not html_doc\n", buf); }

    return Ok;
}


Err doc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]) {
    Err err = Ok;
    buf_append_lit("DocCache: ", buf);
    if (buf_append_hexp(c, buf)) { return "could nor append"; }
    buf_append_lit("\n", buf);

    if (c->hrefs) {
        buf_append_lit(" hrefs: ", buf);
        if (buf_append_hexp(c->hrefs, buf)) { return  "could nor append"; }
        buf_append_lit("\n", buf);

        if (c->hrefs) {
            if((err=lexbor_foreach_href(c->hrefs, ahre_append_href, buf))) {
                return err;
            }
        }
    } else { buf_append_lit(" no hrefs\n", buf); }

    return err;
}
