/* Copyright (C) 2026 Nicolas San Martin

This file is part of ahre.

ahre is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

ahre is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ahre; see the file LICENSE or see
<https://www.gnu.org/licenses/>.  */

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
Err fetch_history_entry_update_title(FetchHistoryEntry e[_1_], Dom np);
Err fetch_history_write_to_file(FetchHistoryEntry e[_1_], FILE* fp);
void fetch_history_entry_clean(FetchHistoryEntry e[_1_]);

#define Hotl_Arl_T FetchHistoryEntry
#define Hotl_Arl_T_Clean fetch_history_entry_clean
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
