#include <stdio.h>

#include <utests.h>

#include <src/range_parse.c>
#include <src/re.c>


//const char* parse_range_impl(char* tk, size_t current_line, size_t len, Range* range);

int test_0(void) {
    //size_t current_line = 0;
    size_t len = 10;
    TextBuf tb = (TextBuf){
        .current_line=0,
        .eols=(ArlOf(size_t)){.items=(size_t[10]){0},len=10,.capacity=10}
    };
    Range range;
    RangeParseCtx ctx = range_parse_ctx_from_textbuf(&tb);

    parse_range_impl("1,2", &ctx, &range);
    utest_assert(range.beg == 1, fail, __LINE__);
    utest_assert(range.end == 2, fail, __LINE__);

    parse_range_impl("0,5", &ctx, &range);
    utest_assert(range.beg == 0, fail, __LINE__);
    utest_assert(range.end == 5, fail, __LINE__);
    
    parse_range_impl("0,15", &ctx, &range);
    utest_assert(range.beg == 0, fail, __LINE__);
    utest_assert(range.end == 5, fail, __LINE__);

    parse_range_impl("0,", &ctx, &range);
    utest_assert(range.beg == 0, fail, __LINE__);
    utest_assert(range.end == 0, fail, __LINE__);

    *textbuf_current_line(&tb) = 2;
    ctx = range_parse_ctx_from_textbuf(&tb);
    parse_range_impl(",4", &ctx, &range);
    utest_assert(range.beg == 2, fail, __LINE__);
    utest_assert(range.end == 4, fail, __LINE__);

    parse_range_impl("%", &ctx, &range);
    utest_assert(range.beg == 1, fail, __LINE__);
    utest_assert(range.end == len + 1, fail, __LINE__);
    return 0;
fail:
    return 1;
}



int main(void) {
    print_running_test(__FILE__);
    int errors = test_0();
    print_test_result(errors);
}
