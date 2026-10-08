#include <utests.h>

#include "../src/cmd-params.h"


static int
test_cmd_match_prefix() {
    CmdParams p = (CmdParams){0};

    utest_assert(cmd_match_prefix(svl("foo bar"), "foo", 3, &p), fail);
    utest_assert(cmd_match_prefix(svl("foo bar"), "foo", 2, &p), fail);
    utest_assert(cmd_match_prefix(svl("foo bar"), "foo", 1, &p), fail);

    utest_assert(!cmd_match_prefix(svl("bar"), "foo", 4, &p), fail);
    utest_assert(!cmd_match_prefix(svl("bar"), "foo", 3, &p), fail);

    utest_assert(!cmd_match_prefix(svl("foa"), "foo", 3, &p), fail);

    utest_assert(cmd_match_prefix(svl("fo"), "foo", 2, &p), fail);

    utest_assert(!cmd_match_prefix(svl("foa"), "foo", 2, &p), fail);

    return 0;
fail:
    return 1;
}



int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_cmd_match_prefix()
        ;
    print_test_result(errors);
}
