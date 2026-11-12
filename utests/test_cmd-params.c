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
