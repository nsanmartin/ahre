#include "src/debug.h"
#include "src/generic.h"
#include "src/htmldoc.h"



/*
 * Functions that append summary of the content for debugging purposes
 */
static Err textbuf_summary(HtmlDoc htmldoc[static 1], BufOf(char)* buf) {

    buf_append_lit("HtmlDoc: ", buf);
    if (buf_append_hexp(htmldoc, buf)) { return "could not append"; }
    buf_append_lit("\n", buf);

    if (htmldoc->url) {
        buf_append_lit(" url: ", buf);
       if (buffn(char,append)(buf, (char*)htmldoc->url, strlen(htmldoc->url))) {
           return  "could not append";
       }
       buf_append_lit("\n", buf);

    } else {
       buf_append_lit(" none\n", buf);

    }
    if (htmldoc->lxbdoc) {
        buf_append_lit(" lxbdoc: ", buf);
        if (buf_append_hexp(htmldoc->lxbdoc, buf)) { return  "could not append"; }
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

    if (session->htmldoc) {
        textbuf_summary(session->htmldoc, buf);
        htmldoc_cache_buffer_summary(&session->htmldoc->cache, buf);
    } else { buf_append_lit("Not htmldoc\n", buf); }

    return Ok;
}


Err htmldoc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]) {
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
