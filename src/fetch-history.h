#ifndef AHRE_FETCH_HISTORY_H__
#define AHRE_FETCH_HISTORY_H__

#include <time.h>

#include <curl/curl.h>

#include "wrapper-lexbor.h"
#include "str.h"
#include "writer.h"

typedef struct {
    struct timespec   ts;
    Str               title;
    Str               effective_url;
    Str               local_ip;
    long              local_port;
    Str               primary_ip;
    long              primary_port;
    curl_off_t        size_download_t;
    curl_off_t        speed_download_t;
    curl_off_t        queue_time_t;
    curl_off_t        namelookup_time_t;
    curl_off_t        connect_time_t;
    curl_off_t        appconnect_time_t;
    curl_off_t        pretransfer_time_t;
    curl_off_t        posttransfer_time_t;
    curl_off_t        starttransfer_time_t;
    curl_off_t        total_time_t;
    curl_off_t        redirect_time_t;
} FetchHistoryEntry;


static inline void fetch_history_entry_clean(FetchHistoryEntry e[_1_]) {
    str_clean(&e->title);
    str_clean(&e->effective_url);
    str_clean(&e->local_ip);
    str_clean(&e->primary_ip);
}

#define T FetchHistoryEntry
#define TClean fetch_history_entry_clean
#include <arl.h>


static inline Err fetch_history_entry_init(FetchHistoryEntry e[_1_]) {
    *e = (FetchHistoryEntry){0};
    if (TIME_UTC != timespec_get(&e->ts, TIME_UTC))
        return "error: timespec_get failure";
    return Ok;
}

Err fetch_history_entry_update_curl(
    FetchHistoryEntry e[_1_],
    CURL*             curl,
    Writer            msg_writer[_1_]
);

Err fetch_history_write_to_file(FetchHistoryEntry e[_1_], FILE* fp);

static inline Err fetch_history_entry_update_title(FetchHistoryEntry e[_1_], LxbNodePtr np[_1_]) {
    try(lexbor_get_title_text(*np, &e->title));
    if (e->title.len) {
        char* rest = e->title.items;
        while ((rest = memchr(rest, '\n', e->title.len - (rest-e->title.items)))) { *rest = ' '; }
    }
    return Ok;
}

#define FETCH_HISTORY_HEADER \
    "timestamp,"          \
    "effective_url,"      \
    "local_ip,"           \
    "local_port,"         \
    "primary_ip,"         \
    "primary_port,"       \
    "size_download,"      \
    "speed_download,"     \
    "queue_time,"         \
    "namelookup_time,"    \
    "connect_time,"       \
    "appconnect_time,"    \
    "pretransfer_time,"   \
    "posttransfer_time,"  \
    "starttransfer_time," \
    "total_time,"         \
    "redirect_time,"     \
    "title\n"              \

#endif
