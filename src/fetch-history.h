#ifndef AHRE_FETCH_HISTORY_H__
#define AHRE_FETCH_HISTORY_H__

#include <time.h>

#include "dom.h"
#include "str.h"
#include "writer.h"
#include "url.h"



typedef struct {
    struct timespec   ts;
    Str               title;
    Str               effective_url;
    Str               local_ip;
    long              local_port;
    Str               primary_ip;
    long              primary_port;
    w_curl_off_t      size_download_t;
    w_curl_off_t      speed_download_t;
    w_curl_off_t      queue_time_t;
    w_curl_off_t      namelookup_time_t;
    w_curl_off_t      connect_time_t;
    w_curl_off_t      appconnect_time_t;
    w_curl_off_t      pretransfer_time_t;
    w_curl_off_t      posttransfer_time_t;
    w_curl_off_t      starttransfer_time_t;
    w_curl_off_t      total_time_t;
    w_curl_off_t      redirect_time_t;
} FetchHistoryEntry;

Err fetch_history_entry_init(FetchHistoryEntry e[_1_]);
Err fetch_history_entry_update_curl(FetchHistoryEntry e[_1_], CURL* curl, CmdOut cmd_out[_1_]);
Err fetch_history_entry_update_title(FetchHistoryEntry e[_1_], DomNode np[_1_]);
Err fetch_history_write_to_file(FetchHistoryEntry e[_1_], FILE* fp);
void fetch_history_entry_clean(FetchHistoryEntry e[_1_]);

#define T FetchHistoryEntry
#define TClean fetch_history_entry_clean
#include <arl.h>



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
    "redirect_time,"      \
    "title\n"             \

#endif
