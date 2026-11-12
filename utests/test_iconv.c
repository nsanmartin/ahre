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
#include <stdio.h>

#include "../src/str.c"

int test_0(void);
int test_1(void);
int test_2(void);
int test_3(void);

int test_0(void) {
    const char* outstr;
    size_t outlen;

    utest_try( mem_convert_to_utf8(NULL, 0,"ISO8859-1", &outstr, &outlen), fail);
    utest_assert(outstr == NULL, fail);
    return 0;
fail:
    return 1;
}

int test_1(void) {
#define T1Str0 "ascii"
    const char* outstr = NULL;
    size_t outlen;

    utest_try( mem_convert_to_utf8(T1Str0, lit_len__(T1Str0),"ISO8859-1",&outstr, &outlen), fail);
    utest_assert(outstr, fail);
    utest_assert(!strncmp(T1Str0, outstr, outlen), fail);
    std_free((void*)outstr);
    return 0;
fail:
    return 1;
}

int test_2(void) {
    const char* outstr = NULL;
    size_t outlen;
    char* t2str0 =  "\xC1\xC9\xCD\xD3\xDA\x0";
    char* t2str0_expected = "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A";
    utest_try( mem_convert_to_utf8(t2str0, strlen(t2str0), "ISO8859-1", &outstr, &outlen), fail);
    utest_assert(outstr, fail_free);
    utest_assert(!strncmp(t2str0_expected, outstr, outlen), fail_free);
    std_free((void*)outstr);
    return 0;
fail_free:
    std_free((void*)outstr);
fail:
    return 1;
}

int test_3(void) {
    const char* outstr = NULL;
    size_t outlen;
    char* t2str0 =  "\xC1\xC9\xCD\xD3\xDA\x0"
                    "\xC1\xC9\xCD\xD3\xDA\x0"
                    "\xC1\xC9\xCD\xD3\xDA\x0"
                    "\xC1\xC9\xCD\xD3\xDA\x0"
                    "\xC1\xC9\xCD\xD3\xDA\x0";
    char* t2str0_expected = "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A"
                            "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A"
                            "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A"
                            "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A"
                            "\xC3\x81\xC3\x89\xC3\x8D\xC3\x93\xC3\x9A"
                            ;
    utest_try( mem_convert_to_utf8(t2str0, strlen(t2str0), "ISO8859-1", &outstr, &outlen), fail);
    utest_assert(outstr, fail_free);
    utest_assert(!strncmp(t2str0_expected, outstr, outlen), fail_free);
    std_free((void*)outstr);
    return 0;
fail_free:
    std_free((void*)outstr);
fail:
    return 1;
}


int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_0()
        + test_1()
        + test_2()
        + test_3()
        ;
    print_test_result(errors);
}
