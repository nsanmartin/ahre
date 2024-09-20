#include <ah/debug.h>

int dbg_print_all_lines_nums(Session session[static 1]) {
    TextBuf* ab = session_current_buf(session);
    BufOf(char)* buf = &ab->buf;
    char* items = buf->items;
    size_t len = buf->len;

    

    for (size_t lnum = 1; /*items && len && lnum < 40*/ ; ++lnum) {
        char* next = memchr(items, '\n', len);
        if (next >= buf->items + buf->len) {
            fprintf(stderr, "Error: print all lines nums\n");
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
    return 0;
}

/*
 * Functions that append summary of the content for debugging purposes
 */
static int textbuf_summary(Doc ahdoc[static 1], BufOf(char)* buf) {

    buf_append_lit("Doc: ", buf);
    if (buf_append_hexp(ahdoc, buf)) { return -1; }
    buf_append_lit("\n", buf);

    if (ahdoc->url) {
        buf_append_lit(" url: ", buf);
       if (buffn(char,append)(buf, (char*)ahdoc->url, strlen(ahdoc->url))) {
           return -1;
       }
       buf_append_lit("\n", buf);

    } else {
       buf_append_lit(" none\n", buf);

    }
    if (ahdoc->doc) {
        buf_append_lit(" doc: ", buf);
        if (buf_append_hexp(ahdoc->doc, buf)) { return -1; }
        buf_append_lit("\n", buf);

    }
    return 0;
}

int url_client_summary(UrlClient ahcurl[static 1], BufOf(char)*buf) {
   buf_append_lit("UrlClient: ", buf);
   if (buf_append_hexp(ahcurl, buf)) { return -1; }
   buf_append_lit("\n", buf);

   if (ahcurl->curl) {
       buf_append_lit(" curl: ", buf);
       if (buf_append_hexp(ahcurl->curl, buf)) { return -1; }
       buf_append_lit("\n", buf);

       if (ahcurl->errbuf[0]) {
           size_t slen = strlen(ahcurl->errbuf);
           if (slen > CURL_ERROR_SIZE) { slen = CURL_ERROR_SIZE; }
           if (buffn(char,append)(buf, ahcurl->errbuf, slen)) {
               return -1;
           }
           buf_append_lit("\n", buf);
       }
   } else {
       buf_append_lit( " not curl\n", buf);
   }
   return 0;
}


int dbg_session_summary(Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (session->ahcurl) {
        if(url_client_summary(session->ahcurl, buf)) { return -1; }
    } else { buf_append_lit("Not ahcurl\n", buf); }

    if (session->ahdoc) {
        textbuf_summary(session->ahdoc, buf);
        ahdoc_cache_buffer_summary(&session->ahdoc->cache, buf);
    } else { buf_append_lit("Not ahdoc\n", buf); }

    return 0;
}


