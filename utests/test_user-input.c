#include <utests.h>

#include "../src/user-input.h"



static int
test_user_line_unskip() {
    UserLine ul = (UserLine){.full="",.len=0,.ix=0};
    user_line_unskip(&ul, 0);
    utest_assert(ul.ix == 0, fail);

    ul = (UserLine){.full="01234",.len=5,.ix=0};
    user_line_unskip(&ul, 0);
    utest_assert(ul.ix == 0, fail);

    ul.ix = 2;
    user_line_unskip(&ul, 1);
    utest_assert(ul.ix == 1, fail);
    user_line_unskip(&ul, 1);
    utest_assert(ul.ix == 0, fail);

    ul.ix = 3;
    user_line_unskip(&ul, 5);
    utest_assert(ul.ix == 0, fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_skip() {
    UserLine ul = (UserLine){.full="",.len=0,.ix=0};
    user_line_skip(&ul, 0);
    utest_assert(ul.ix == 0, fail);
    user_line_skip(&ul, 3);
    utest_assert(ul.ix == 0, fail);

    ul = (UserLine){.full="01234",.len=5,.ix=0};
    user_line_skip(&ul, 0);
    utest_assert(ul.ix == 0, fail);
    user_line_skip(&ul, 3);
    utest_assert(ul.ix == 3, fail);
    user_line_skip(&ul, 3);
    utest_assert(ul.ix == 5, fail);

    ul = (UserLine){.full="01234",.len=5,.ix=0};
    user_line_skip(&ul, 999);
    utest_assert(ul.ix == 5, fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_skip_space() {
    UserLine ul = (UserLine){.full="",.len=0,.ix=0};
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 0, fail);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 0, fail);

    ul = (UserLine){.full="   123",.len=6,.ix=0};
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);

    user_line_skip(&ul, 3);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 6, fail);

    ul = (UserLine){.full="x  123",.len=6,.ix=0};
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 0, fail);
    user_line_skip(&ul, 3);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_match_char() {
    UserLine ul = (UserLine){.full="01234",.len=5,.ix=0};

    utest_assert(user_line_match_char(&ul, '0'), fail);
    utest_assert(user_line_match_char(&ul, '1'), fail);
    utest_assert(user_line_match_char(&ul, '2'), fail);
    utest_assert(user_line_match_char(&ul, '3'), fail);
    utest_assert(user_line_match_char(&ul, '4'), fail);
    utest_assert(!user_line_match_char(&ul, '5'), fail);

    ul = (UserLine){.full="01234",.len=5,.ix=0};
    utest_assert(!user_line_match_char(&ul, '1'), fail);
    utest_assert(!user_line_match_char(&ul, '2'), fail);
    utest_assert(!user_line_match_char(&ul, '3'), fail);
    utest_assert(!user_line_match_char(&ul, '4'), fail);
    utest_assert(!user_line_match_char(&ul, '5'), fail);
    utest_assert(user_line_match_char(&ul, '0'), fail);

    ul = (UserLine){.full="01234",.len=5,.ix=0};
    utest_assert(user_line_match_char(&ul, '0'), fail);
    utest_assert(!user_line_match_char(&ul, '0'), fail);
    utest_assert(!user_line_match_char(&ul, '0'), fail);

    return 0;
fail:
    return 1;
}


int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_user_line_match_char()
        + test_user_line_unskip()
        + test_user_line_skip()
        + test_user_line_skip_space()
        ;
    print_test_result(errors);
}
