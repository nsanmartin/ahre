#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#define FAILED(msg) { perror(msg); exit(EXIT_FAILURE); }

size_t lexbor_callback(char *in, size_t size, size_t nmemb, void* outstream);

lxb_inline lxb_status_t serializer_callback(const lxb_char_t *data, size_t len, void *ctx) {
    (void)ctx;
    return len == fwrite(data, 1, len, stdout) ? LXB_STATUS_OK : LXB_STATUS_ERROR;
}

lxb_inline void
serialize_node(lxb_dom_node_t *node)
{
    lxb_status_t status;

    status = lxb_html_serialize_pretty_cb(node, LXB_HTML_SERIALIZE_OPT_UNDEF,
                                          0, serializer_callback, NULL);
    if (status != LXB_STATUS_OK) {
        FAILED("Failed to serialization HTML tree");
    }
}



lxb_inline lxb_status_t serialize(lxb_dom_node_t *node) {
    return lxb_html_serialize_pretty_tree_cb(
        node, LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL
    );
}

size_t lexbor_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    lxb_html_document_t* document = outstream;
    size_t r = size * nmemb;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}
 
size_t tidy_callback(char *in, size_t size, size_t nmemb, void* out) {
    //TidyBuffer *out) {
  unsigned r = size * nmemb;
  tidyBufAppend(out, in, r);
  return r;
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
          //fprintf(stderr, "%s\n", tidy_errbuf.bp); /* show errors */
        }
      }
    }
    return err;
}
 
int curl_set_all_options(CURL* curl, const char* url, char* errbuf) {
    return curl_easy_setopt(curl, CURLOPT_URL, url)
        || curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf)
        || curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L)
        || curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)
        ;
}

int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf) {
    return curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback)
        || curl_easy_setopt(curl, CURLOPT_WRITEDATA, docbuf)
        ;
}

typedef struct {
    CURL* curl;
    char errbuf[CURL_ERROR_SIZE];
} CurlWithErrors;

CurlWithErrors curl_create(void) {
    CurlWithErrors rv = (CurlWithErrors){
        .curl=curl_easy_init(),
        .errbuf={0}
    };
    return rv;
}

int ahre_tidy(const char* url) {
        CurlWithErrors cwe = curl_create();
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

int lexbor_fetch_document(lxb_html_document_t* document, const char* url) {

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(document)) { FAILED("Failed to parse HTML"); }

    CurlWithErrors cwe = curl_create();
    if (!cwe.curl) { FAILED("curl_easy_init failure\n"); }

    if (curl_set_all_options(cwe.curl, url, cwe.errbuf)
        || curl_set_callback_and_buffer(cwe.curl, lexbor_callback, document)) {
        FAILED("error initializing cwe.curl");
    }

    if (curl_easy_perform(cwe.curl)) { FAILED(cwe.errbuf); }

    curl_easy_cleanup(cwe.curl);

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(document)) { FAILED("Failed to parse HTML"); }
    return 0;
}

int lexbor_print_a_href(lxb_html_document_t* document) {
    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 128);
    if (collection == NULL) {
        FAILED("Failed to create Collection object");
    }

    if (LXB_STATUS_OK !=
        lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body), collection, (const lxb_char_t *) "a", 1)
    ) { FAILED("Failed to get elements by name"); }

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);

        //serialize_node(lxb_dom_interface_node(element));
        size_t value_len = 0;
        const lxb_char_t * value = lxb_dom_element_get_attribute(element, (const lxb_char_t*)"href", 4, &value_len);
        fwrite(value, 1, value_len, stdout);
        fwrite("\n", 1, 1, stdout);
    }

    lxb_dom_collection_destroy(collection, true);
    return 0;
}

int ahre_lexbor(const char* url) {
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) { FAILED("Failed to create HTML Document"); }

    if (lexbor_fetch_document(document, url)) { FAILED("Failed to create HTML Document"); }

    //if (LXB_STATUS_OK != serialize(lxb_dom_interface_node(document))) { FAILED("Failed to serialize HTML"); }
    lexbor_print_a_href(document);


    lxb_html_document_destroy(document);
    return EXIT_SUCCESS;
}

void print_help(const char* program) {
    printf("usage: %s [-b (lxb|tidy)] <url>\n", program);
}

typedef enum { LxbBe, TidyBe } BeLib;

int main(int argc, char **argv) {
    bool bad_input = false;
    if (argc != 2 && argc != 4) { goto exit_error; }

    BeLib be_lib = LxbBe; /* default is lexbor */

    size_t param = 1;
    if  (argc == 4) {
        if (strcmp("-b", argv[param]) == 0) {
            if (strcmp("lxb", argv[param+1]) == 0) {
                be_lib = LxbBe;
                goto opt_read;
            } else if (strcmp("tidy", argv[param+1]) == 0) {
                be_lib = TidyBe;
                goto opt_read;
            } 
        } 
exit_error:
        print_help(argv[0]);
        exit(EXIT_FAILURE); 
opt_read:
        param += 2;
    }

    if (!bad_input) {
        switch (be_lib) {
            case LxbBe: { return ahre_lexbor(argv[param]); }
            case TidyBe: { return ahre_tidy(argv[param]); }
            default: { goto exit_error; }
        }
    }

}

