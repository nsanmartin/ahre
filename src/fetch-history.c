#include "fetch-history.h"

Err fetch_history_entry_update_curl(
    FetchHistoryEntry e[_1_],
    CURL*             curl,
    Writer            msg_writer[_1_]
) {
    const char* effective_url;
    const char* local_ip;
    const char* primary_ip;

    CURLcode c = CURLE_OK;
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_LOCAL_IP, &local_ip);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_LOCAL_PORT, &e->local_port);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_PRIMARY_IP, &primary_ip);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_PRIMARY_PORT, &e->primary_port);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &e->size_download_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &e->speed_download_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_QUEUE_TIME_T, &e->queue_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME_T, &e->namelookup_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME_T, &e->connect_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME_T, &e->appconnect_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME_T, &e->pretransfer_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_POSTTRANSFER_TIME_T, &e->posttransfer_time_t);
    if(c==CURLE_OK)c=curl_easy_getinfo(curl,CURLINFO_STARTTRANSFER_TIME_T, &e->starttransfer_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &e->total_time_t);
    if(c==CURLE_OK) c=curl_easy_getinfo(curl, CURLINFO_REDIRECT_TIME_T, &e->redirect_time_t);

    if (c !=CURLE_OK ) {
        const char* cerr = curl_easy_strerror(c);
        try(writer_write(msg_writer,(char*)cerr, strlen(cerr))); 
    } else {
        str_append(&e->effective_url, (char*)effective_url, strlen(effective_url));
        str_append(&e->local_ip,      (char*)local_ip,      strlen(local_ip));
        str_append(&e->primary_ip,    (char*)primary_ip,    strlen(primary_ip));
    }
    return Ok;
}


Err fetch_history_write_to_file(FetchHistoryEntry e[_1_], FILE* fp) {
    Str* dt = &(Str){0};
    Err err = str_append_timespec(dt, &e->ts);
    if (err) {
        str_clean(dt);
        return err;
    }
    try(file_write_lit_sep(items__(dt), len__(dt), ",", fp));
    str_clean(dt);

    try(file_write_lit_sep(e->effective_url.items, e->effective_url.len, ",", fp));
    try(file_write_lit_sep(e->local_ip.items, e->local_ip.len, ",", fp));
    try(file_write_int_lit_sep(e->local_port, ",", fp));
    try(file_write_lit_sep(e->primary_ip.items, e->primary_ip.len, ",", fp));

    try(file_write_int_lit_sep(e->primary_port, ",", fp));
    try(file_write_int_lit_sep(e->size_download_t, ",", fp));
    try(file_write_int_lit_sep(e->speed_download_t, ",", fp));
    try(file_write_int_lit_sep(e->queue_time_t, ",", fp));
    try(file_write_int_lit_sep(e->namelookup_time_t, ",", fp));
    try(file_write_int_lit_sep(e->connect_time_t, ",", fp));
    try(file_write_int_lit_sep(e->appconnect_time_t, ",", fp));
    try(file_write_int_lit_sep(e->pretransfer_time_t, ",", fp));
    try(file_write_int_lit_sep(e->posttransfer_time_t, ",", fp));
    try(file_write_int_lit_sep(e->starttransfer_time_t, ",", fp));
    try(file_write_int_lit_sep(e->total_time_t, ",", fp));
    try(file_write_int_lit_sep(e->redirect_time_t, ",", fp));
    try(file_write_lit_sep(e->title.items, e->title.len, "\n", fp));
    return Ok;
}
