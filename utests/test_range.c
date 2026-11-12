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

#include <stdio.h>

#include <utests.h>

#include "../src/range-parse.c"
#include "../src/re.c"

int test_1(void);



int test_1(void) {
    const char* tk;
    const char* endptr;
    RangeParse res;

    tk = "";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_none_tag, fail);
    utest_assert(res.end.tag == range_addr_none_tag, fail);

    tk = "^";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_prev_tag, fail);
    utest_assert(res.end.tag == range_addr_prev_tag, fail);

    tk = "%";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_beg_tag, fail);
    utest_assert(res.end.tag == range_addr_end_tag, fail);

    tk = ",";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_none_tag, fail);

    tk = "13";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_num_tag, fail);
    utest_assert(res.beg.n == 13, fail);
    utest_assert(res.end.tag == range_addr_none_tag, fail);

    tk = "13,15";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_num_tag, fail);
    utest_assert(res.beg.n == 13, fail);
    utest_assert(res.end.tag == range_addr_num_tag, fail);
    utest_assert(res.end.n == 15, fail);

    tk = ",$";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_end_tag, fail);

    tk = "-,+";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -1, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == +1, fail);

    tk = ".-2,.+3";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -2, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == +3, fail);

    tk = "/foo/,/barba/";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_search_tag, fail);
    utest_assert(res.beg.s.items == tk+1, fail);
    utest_assert(res.beg.s.len == 3, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.len == 5, fail);

    tk = "/foo/+11,/barba/-30";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_search_tag, fail);
    utest_assert(res.beg.s.items == tk+1, fail);
    utest_assert(res.beg.s.len == 3, fail);
    utest_assert(res.beg.delta == 11, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+10, fail);
    utest_assert(res.end.s.len == 5, fail);
    utest_assert(res.end.delta == -30, fail);

    tk = "-7,+1000";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -7, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == 1000, fail);

    tk = ".-7,.+1000";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -7, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == 1000, fail);

    tk = ",/foo";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+2, fail);
    utest_assert(res.end.s.len == 3, fail);

    tk = ",/foo-4";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+2, fail);
    utest_assert(res.end.s.len == 5, fail);
    utest_assert(res.end.delta == 0, fail);

    tk = ",/foo/-4";
    parse_range(tk, &res, &endptr, 10);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+2, fail);
    utest_assert(res.end.s.len == 3, fail);
    utest_assert(res.end.delta == -4, fail);

    return 0;
fail:
    return 1;
}

int main(void) {
    print_running_test(__FILE__);
    int errors = test_1();
    print_test_result(errors);
}
