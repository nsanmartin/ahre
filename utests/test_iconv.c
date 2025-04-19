#include <stdio.h>
#include <utests.h>

#include "../src/str.c"

int test_0(void) {
    const char* outstr;
    size_t outlen;

    utest_try( _convert_to_utf8_(NULL, 0,"ISO8859-1", &outstr, &outlen), fail);
    utest_assert(outstr == NULL, fail);
    return 0;
fail:
    return 1;
}

int test_1(void) {
#define T1Str0 "ascii"
    const char* outstr = NULL;
    size_t outlen;

    utest_try( _convert_to_utf8_(T1Str0, lit_len__(T1Str0),"ISO8859-1",&outstr, &outlen), fail);
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
    utest_try( _convert_to_utf8_(t2str0, strlen(t2str0), "ISO8859-1", &outstr, &outlen), fail);
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
    utest_try( _convert_to_utf8_(t2str0, strlen(t2str0), "ISO8859-1", &outstr, &outlen), fail);
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
