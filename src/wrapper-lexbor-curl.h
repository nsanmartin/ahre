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

#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "wrapper-lexbor.h"
#include "wrapper-curl.h"

Err w_curl_set_url(CurlPtr curl, Url url[_1_]);

static inline CURLoption curlopt_method_from_http_method(HttpMethod m) {
    return m == http_post ? CURLOPT_POST : CURLOPT_HTTPGET ;
}


#endif 
