#include <stdio.h>

#include <utests.h>

#include "../src/range-parse.c"
#include "../src/re.c"


//const char* parse_range_impl(char* tk, size_t current_line, size_t len, Range* range);

/* int test_0(void) { */
/*     size_t len = 10; */
/*     char* items = "1\n2\n3\n4\n5\n6\n7\n8\n9\nA"; */
/*     TextBuf tb = (TextBuf){ */
/*         .buf          = (BufOf(char)){.items=items, .len=strlen(items)}, */
/*         .eols         = (ArlOf(size_t)){ */
/*             .items=(size_t[10]){ 1, 3, 5, 7, 9, 11, 13, 15, 17}, */
/*             .len=10, */
/*             .capacity=10 */
/*         } */
/*     }; */
/*     Range range; */
/*     RangeParseCtx ctx = range_parse_ctx_from_textbuf(&tb); */

/*     parse_range_impl("1,2", &ctx, &range); */
/*     utest_assert(range.beg == 1, fail); */
/*     utest_assert(range.end == 2, fail); */

/*     parse_range_impl("0,5", &ctx, &range); */
/*     utest_assert(range.beg == 0, fail); */
/*     utest_assert(range.end == 5, fail); */

/*     parse_range_impl("0,15", &ctx, &range); */
/*     utest_assert(range.beg == 0, fail); */
/*     utest_assert(range.end == 5, fail); */

/*     parse_range_impl("0,", &ctx, &range); */
/*     utest_assert(range.beg == 0, fail); */
/*     utest_assert(range.end == 0, fail); */

/*     *textbuf_current_offset(&tb) = 2; */
/*     ctx = range_parse_ctx_from_textbuf(&tb); */
/*     parse_range_impl(",4", &ctx, &range); */
/*     utest_assert(range.beg == 2, fail); */
/*     utest_assert(range.end == 4, fail); */

/*     parse_range_impl("%", &ctx, &range); */
/*     utest_assert(range.beg == 1, fail); */
/*     utest_assert(range.end == len + 1, fail); */
/*     return 0; */
/* fail: */
/*     return 1; */
/* } */



int test_1(void) {
    const char* tk;
    const char* endptr;
    RangeParse res;

    tk = "";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_none_tag, fail);
    utest_assert(res.end.tag == range_addr_none_tag, fail);

    tk = "^";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_prev_tag, fail);
    utest_assert(res.end.tag == range_addr_prev_tag, fail);

    tk = "%";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_beg_tag, fail);
    utest_assert(res.end.tag == range_addr_end_tag, fail);

    tk = ",";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_none_tag, fail);

    tk = "13";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_num_tag, fail);
    utest_assert(res.beg.n == 13, fail);
    utest_assert(res.end.tag == range_addr_none_tag, fail);

    tk = "13,15";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_num_tag, fail);
    utest_assert(res.beg.n == 13, fail);
    utest_assert(res.end.tag == range_addr_num_tag, fail);
    utest_assert(res.end.n == 15, fail);

    tk = ",$";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_end_tag, fail);

    tk = "-,+";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -1, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == +1, fail);

    tk = ".-2,.+3";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -2, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == +3, fail);

    tk = "/foo/,/barba/";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_search_tag, fail);
    utest_assert(res.beg.s.items == tk+1, fail);
    /* Note that we use a string view for the pattern that includes one
     * byte more than the last one, ie, the one reserved for the \0 byte.
     * But it does not have '\0' necessarily.
     */
    utest_assert(res.beg.s.len == 4, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.len == 6, fail);

    tk = "/foo/+11,/barba/-30";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_search_tag, fail);
    utest_assert(res.beg.s.items == tk+1, fail);
    utest_assert(res.beg.s.len == 4, fail);
    utest_assert(res.beg.delta == 11, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+10, fail);
    utest_assert(res.end.s.len == 6, fail);
    utest_assert(res.end.delta == -30, fail);

    tk = "-7,+1000";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -7, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == 1000, fail);

    tk = ".-7,.+1000";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.beg.delta == -7, fail);
    utest_assert(res.end.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.delta == 1000, fail);

    tk = ",/foo";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+2, fail);
    utest_assert(res.end.s.len == 4, fail);

    tk = ",/foo-4";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+2, fail);
    utest_assert(res.end.s.len == 6, fail);
    utest_assert(res.end.delta == 0, fail);

    tk = ",/foo/-4";
    parse_range(tk, &res, &endptr);
    utest_assert(res.beg.tag == range_addr_curr_tag, fail);
    utest_assert(res.end.tag == range_addr_search_tag, fail);
    utest_assert(res.end.s.items == tk+2, fail);
    utest_assert(res.end.s.len == 4, fail);
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
