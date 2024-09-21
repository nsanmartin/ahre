#include <ah/debug.h>

Err dbg_print_all_lines_nums(Session session[static 1]) {
    TextBuf* ab = session_current_buf(session);
    BufOf(char)* buf = &ab->buf;
    char* items = buf->items;
    size_t len = buf->len;

    

    for (size_t lnum = 1; /*items && len && lnum < 40*/ ; ++lnum) {
        char* next = memchr(items, '\n', len);
        if (next >= buf->items + buf->len) {
            fprintf(stderr, "Error: print all lines nums\n");
            return  "Error: print all lines nums.";
        }
        printf("%ld: ", lnum);

        if (next) {
            size_t line_len = 1+next-items;
            fwrite(items, 1, line_len, stdout);
            items += line_len;
            len -= line_len;
        } else {
            fwrite(items, 1, len, stdout);
            break;
        }
    }
    return Ok;
}

/*
 * Functions that append summary of the content for debugging purposes
 */
static Err textbuf_summary(Doc doc[static 1], BufOf(char)* buf) {

    buf_append_lit("Doc: ", buf);
    if (buf_append_hexp(doc, buf)) { return "could not append"; }
    buf_append_lit("\n", buf);

    if (doc->url) {
        buf_append_lit(" url: ", buf);
       if (buffn(char,append)(buf, (char*)doc->url, strlen(doc->url))) {
           return  "could not append";
       }
       buf_append_lit("\n", buf);

    } else {
       buf_append_lit(" none\n", buf);

    }
    if (doc->lxbdoc) {
        buf_append_lit(" lxbdoc: ", buf);
        if (buf_append_hexp(doc->lxbdoc, buf)) { return  "could not append"; }
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

    if (session->doc) {
        textbuf_summary(session->doc, buf);
        doc_cache_buffer_summary(&session->doc->cache, buf);
    } else { buf_append_lit("Not doc\n", buf); }

    return Ok;
}


