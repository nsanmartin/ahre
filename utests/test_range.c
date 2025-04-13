#include <stdio.h>

#include <utests.h>

#include "../src/range-parse.c"
#include "../src/re.c"


//const char* parse_range_impl(char* tk, size_t current_line, size_t len, Range* range);

int test_0(void) {
    size_t len = 10;
    char* items = "1\n2\n3\n4\n5\n6\n7\n8\n9\nA";
    TextBuf tb = (TextBuf){
        .buf          = (BufOf(char)){.items=items, .len=strlen(items)},
        .eols         = (ArlOf(size_t)){
            .items=(size_t[10]){ 1, 3, 5, 7, 9, 11, 13, 15, 17},
            .len=10,
            .capacity=10
        }
    };
    Range range;
    RangeParseCtx ctx = range_parse_ctx_from_textbuf(&tb);

    parse_range_impl("1,2", &ctx, &range);
    utest_assert(range.beg == 1, fail);
    utest_assert(range.end == 2, fail);

    parse_range_impl("0,5", &ctx, &range);
    utest_assert(range.beg == 0, fail);
    utest_assert(range.end == 5, fail);
    
    parse_range_impl("0,15", &ctx, &range);
    utest_assert(range.beg == 0, fail);
    utest_assert(range.end == 5, fail);

    parse_range_impl("0,", &ctx, &range);
    utest_assert(range.beg == 0, fail);
    utest_assert(range.end == 0, fail);

    *textbuf_current_offset(&tb) = 2;
    ctx = range_parse_ctx_from_textbuf(&tb);
    parse_range_impl(",4", &ctx, &range);
    utest_assert(range.beg == 2, fail);
    utest_assert(range.end == 4, fail);

    parse_range_impl("%", &ctx, &range);
    utest_assert(range.beg == 1, fail);
    utest_assert(range.end == len + 1, fail);
    return 0;
fail:
    return 1;
}



int main(void) {
    print_running_test(__FILE__);
    int errors = test_0();
    print_test_result(errors);
}
