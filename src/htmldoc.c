#include "src/textbuf.h"
#include "src/htmldoc.h"
#include "src/error.h"
#include "src/generic.h"
#include "src/mem.h"
#include "src/utils.h"

/* internal linkage */
//static constexpr size_t MAX_URL_LEN = 2048;
#define MAX_URL_LEN 2048
//static constexpr size_t READ_FROM_FILE_BUFFER_LEN = 4096;
#define READ_FROM_FILE_BUFFER_LEN 4096
_Thread_local static unsigned char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {0};

static Err browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, void* ctx);
static Err
attr_href(lxb_dom_node_t *node, lxb_html_serialize_cb_f cb, void *ctx);

static lxb_status_t
serialize_cb(const lxb_char_t *data, size_t len, void *ctx) ;

static TextBuf* htmldoc_textbuf(HtmlDoc htmldoc[static 1]) { return &htmldoc->textbuf; }

//TODO: use str ndup cstr
static char* str_url_dup(const Str* url) {
    if (str_is_empty(url)) { return 0x0; }
    if (len(url) >= MAX_URL_LEN) {
        perror("url too long");
        return 0x0;
    }

    char* res = malloc(len(url) + 1);
    if (!res) { return NULL; }
    res[len(url)] = '\0';
    memcpy(res, url->s, len(url));
    return res;
}


static lxb_status_t
serialize_cb(const lxb_char_t *data, size_t len, void *ctx) {
    //(void)ctx;
    //fwrite(data, 1, len, stdout);
    //return LXB_STATUS_OK;
    TextBuf* textbuf = htmldoc_textbuf(ctx);
    return textbuf_append_part(textbuf, (char*)data, len)
        ? LXB_STATUS_ERROR
        : LXB_STATUS_OK;
}


static Err
attr_href(lxb_dom_node_t *node, lxb_html_serialize_cb_f cb, void *ctx)
{
    lxb_dom_attr_t *attr;
    const lxb_char_t *data;
    size_t data_len;

    attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));

    while (attr != NULL) {
        data = lxb_dom_attr_qualified_name(attr, &data_len);

        if (!strcmp("href", (char*)data))  {
            //if(LXB_STATUS_OK != cb(data, data_len, ctx)) { return "error serializing data"; }

            data = lxb_dom_attr_value(attr, &data_len);

            if (data != NULL) {
                if (
                    LXB_STATUS_OK != cb((const lxb_char_t *) "[", 1, ctx)
                    || LXB_STATUS_OK != cb(data, data_len, ctx)
                    || LXB_STATUS_OK != cb((const lxb_char_t *) "]", 1, ctx)
                ) { return "error serializing data"; }


            }

            return Ok;

        }
        attr = lxb_dom_element_next_attribute(attr);
    }

    return Ok;
}



static Err browse_tag_a(lxb_dom_node_t *node, lxb_html_serialize_cb_f cb, void* ctx) {
    cb((lxb_char_t*)EscCodeBlue, sizeof EscCodeRed, ctx);
    lxb_dom_node_t* it = node->first_child;
    const lxb_dom_node_t* last = node->last_child; 
    for(; ; it = it->next) {
        Err err = browse_rec(it, cb, ctx);
        if (err) return err;
        if (it == last) { break; }
    }
    attr_href(node, cb, ctx);
    cb((lxb_char_t*)EscCodeReset, sizeof EscCodeReset, ctx);
    return Ok;

}

static Err browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, void* ctx) {
    if (node) {
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {

            if (node->local_name == LXB_TAG_SCRIPT) { printf("skip script\n"); } 

            if (node->local_name == LXB_TAG_A) {
                return browse_tag_a(node, cb, ctx);
            }
        } else if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
            size_t len = lxb_dom_interface_text(node)->char_data.data.length;
            unsigned char* data = lxb_dom_interface_text(node)->char_data.data.data;
            if (!is_all_space((char*)data, len)) {
                cb(data, len, ctx);
            } //else cb((lxb_char_t*)"\n", 1, ctx);
            
        }

        lxb_dom_node_t* it = node->first_child;
        const lxb_dom_node_t* last = node->last_child; 

        for(; /*it != last*/ ; it = it->next) {
            browse_rec(it, cb, ctx);
            if (it == last) { break; }
        }

    }   

    return Ok;
}

/* external linkage */

Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) {
    FILE* fp = fopen(htmldoc->url, "r");
    if  (!fp) { return strerror(errno); }

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(htmldoc->lxbdoc)) {
        return "Lex failed to init html document";
    }

    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if (LXB_STATUS_OK != lxb_html_document_parse_chunk(
                htmldoc->lxbdoc,
                read_from_file_buffer,
                bytes_read
            )
        ) {
            return "Failed to parse HTML chunk";
        }
    }
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(htmldoc->lxbdoc)) {
        return "Lbx failed to parse html";
    }
    return 0x0;
}




int htmldoc_init(HtmlDoc d[static 1], const Str url[static 1]) {
    if (!d) { return -1; }
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        return -1;
    }

    const char* u = NULL;
    if (!str_is_empty(url)){
        u = str_url_dup(url);
        if (!u) {
            lxb_html_document_destroy(d->lxbdoc);
            return -1;
        }
    }

    *d = (HtmlDoc){ .url=u, .lxbdoc=document, .textbuf=(TextBuf){.current_line=1} };
    return 0;
}

HtmlDoc* htmldoc_create(const char* url) {
    HtmlDoc* rv = std_malloc(sizeof(HtmlDoc));
    Str u;
    if (str_init(&u, url)) { return NULL; }
    if (htmldoc_init(rv, &u)) {
        destroy(rv);
        return NULL;
    }
    return rv;
}

void htmldoc_reset(HtmlDoc htmldoc[static 1]) {
    htmldoc_cache_cleanup(&htmldoc->cache);
    lxb_html_document_clean(htmldoc->lxbdoc);
    textbuf_reset(&htmldoc->textbuf);
    destroy((char*)htmldoc->url);
}

void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) {
    htmldoc_cache_cleanup(&htmldoc->cache);
    lxb_html_document_destroy(htmldoc->lxbdoc);
    textbuf_cleanup(&htmldoc->textbuf);
    destroy((char*)htmldoc->url);
}

inline void htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}


inline void htmldoc_update_url(HtmlDoc ad[static 1], char* url) {
        destroy((char*)ad->url);
        ad->url = url;
}


bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    return htmldoc->url && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

Err htmldoc_read_from_file(HtmlDoc htmldoc[static 1]) {
    FILE* fp = fopen(htmldoc->url, "r");
    if  (!fp) { return strerror(errno); }

    Err err = NULL;

    TextBuf* textbuf = htmldoc_textbuf(htmldoc);
    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if ((err = textbuf_append_part(textbuf, (char*)read_from_file_buffer, READ_FROM_FILE_BUFFER_LEN))) {
            return err;
        }
    }
    //TODO: free mem?
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    return textbuf_append_null(textbuf);
}


Err htmldoc_browse(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    //return browse_rec(lxb_dom_interface_node(lxbdoc), serialize_cb, htmldoc);
    Err err = browse_rec(lxb_dom_interface_node(lxbdoc), serialize_cb, htmldoc);
    if (err) return err;
    return textbuf_append_null(htmldoc_textbuf(htmldoc));
}


