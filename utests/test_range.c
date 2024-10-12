#include <stdio.h>

#include <utests.h>

#include <src/ranges.c>
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

    parse_range_impl("1,2", &tb, &range);
    utest_assert(range.beg == 1, fail);
    utest_assert(range.end == 2, fail);

    parse_range_impl("0,5", &tb, &range);
    utest_assert(range.beg == 0, fail);
    utest_assert(range.end == 5, fail);
    
    parse_range_impl("0,15", &tb, &range);
    utest_assert(range.beg == 0, fail);
    utest_assert(range.end == 15, fail);

    parse_range_impl("0,", &tb, &range);
    utest_assert(range.beg == 0, fail);
    utest_assert(range.end == 0, fail);

    *textbuf_current_line(&tb) = 2;
    parse_range_impl(",4", &tb, &range);
    utest_assert(range.beg == 2, fail);
    utest_assert(range.end == 4, fail);

    parse_range_impl("%", &tb, &range);
    utest_assert(range.beg == 1, fail);
    utest_assert(range.end == len, fail);
    return 0;
fail:
    return -1;
}



int main(void) {
    int errors = test_0();
    if (errors) {
        fprintf(stderr, "%d errors\n", errors);
    } else {
        puts("Tests Passed");
    }
}
