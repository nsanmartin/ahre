#include <stdio.h>

#include <utests.h>

#include <src/error.c>


int test_0(void) {
    const char* f0 = "foo";
    size_t c0 = count_specifiers(f0);
    utest_assert(c0 == 0, fail);

    const char* f1 = "foo%sbar";
    size_t c1 = count_specifiers(f1);
    utest_assert(c1 == 1, fail);

    const char* f2 = "foo%s%sbar";
    size_t c2 = count_specifiers(f2);
    utest_assert(c2 == 2, fail);

    const char* f3 = "foo%s%sbar%s";
    size_t c3 = count_specifiers(f3);
    utest_assert(c3 == 3, fail);

    const char* f4 = "foo%s%sbar%s%";
    size_t c4 = count_specifiers(f4);
    utest_assert(c4 == 3, fail);

    const char* f5 = "%%%%%%%%%%%%%%%%%%";
    size_t c5 = count_specifiers(f5);
    utest_assert(c5 == 0, fail);


    return 0;
fail:
    return -1;
}


int test_1(void) {
    Err e0 = err_fmt("FOO");
    utest_assert(strcmp(e0, "FOO") == 0, fail);

    Err e1 = err_fmt("FOO%sBAR", "__");
    utest_assert(strcmp(e1, "FOO__BAR") == 0, fail);

    return 0;
fail:
    return -1;
}



int main(void) {
    int errors = test_0()
              || test_1();
    if (errors) {
        fprintf(stderr, "%d errors\n", errors);
    } else {
        puts("Tests Passed");
    }
}
