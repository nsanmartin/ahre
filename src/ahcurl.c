#include <string.h>
#include <ahcurl.h>

int ahcurl_buffer_summary(AhCurl ahcurl[static 1], BufOf(char)*buf) {
   buf_append_lit("AhCurl: ", buf);
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


