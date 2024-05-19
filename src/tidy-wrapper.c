#include <wrappers.h>

int tidy_parse(TidyDoc* tdocp, TidyBuffer* docbuf);


int curl_set_all_options(CURL* curl, const char* url, char* errbuf) {
    return curl_easy_setopt(curl, CURLOPT_URL, url)
        || curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf)
        || curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L)
        || curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)
        ;
}


int ah_tidy(const char* url) {
        AhCurl cwe = curl_create();
        if (!cwe.curl) {
            fprintf(stderr, "curl_easy_init failure\n");
            return -1;
        }

        TidyBuffer docbuf = {0};
        int err = curl_set_all_options(cwe.curl, url, cwe.errbuf)
            || curl_set_callback_and_buffer(cwe.curl, tidy_callback, &docbuf);
        if (err) {
            fprintf(stderr, "error initializing cwe.curl");
            return -1;
        }
   
        TidyDoc tdoc;
        TidyBuffer tidy_errbuf = {0};
        tdoc = tidyCreate();
        err = !tidyOptSetBool(tdoc, TidyForceOutput, yes)
            && !tidyOptSetInt(tdoc, TidyWrapLen, 4096);
        if (err) { fprintf(stderr, "error tidy opt set"); return -1; }

        err = tidySetErrorBuffer(tdoc, &tidy_errbuf);
        if (err) { fprintf(stderr, "tidy error setting buffer"); return -1; }
        /* void */ tidyBufInit(&docbuf);
   
        err = curl_easy_perform(cwe.curl);
        if(!err) {
            tidy_parse(&tdoc, &docbuf);
            tidyBufFree(&docbuf);
            tidyBufFree(&tidy_errbuf);
  
        }
        else {
          fprintf(stderr, "%s\n", cwe.errbuf);
        }
   
        tidyRelease(tdoc);
        curl_easy_cleanup(cwe.curl);
        return err;
}



void dump_hrefs(TidyDoc doc, TidyNode tnod, int indent) {
    TidyNode child;
    for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
        ctmbstr name = tidyNodeGetName(child);
        if(name) {
            TidyAttr attr;
            for(attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr) ) {
                if (strcmp("a", name) == 0
                        && strcmp("href", tidyAttrName(attr)) == 0) {
                    if (tidyAttrValue(attr)) { 
                        printf("%s\n", tidyAttrValue(attr));
                    }
                }
            }
        }
        dump_hrefs(doc, child, indent + 4);
    }
}

 
int tidy_parse(TidyDoc* tdocp, TidyBuffer* docbuf) {
    TidyDoc tdoc = *tdocp;
    int err = tidyParseBuffer(tdoc, docbuf); /* parse the input */
    if(err >= 0) {
      err = tidyCleanAndRepair(tdoc); /* fix any problems */
      if(err >= 0) {
        err = tidyRunDiagnostics(tdoc); /* load tidy error buffer */
        if(err >= 0) {
          dump_hrefs(tdoc, tidyGetRoot(tdoc), 0); /* walk the tree */
        }
      }
    }
    return err;
}
