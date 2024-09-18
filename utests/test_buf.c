#include <stdio.h>

#include <utests.h>
#include <ah/buf.h>



int test_0(void) {

    char* str = "01\n02\n03";
    size_t len = 8;

    size_t count = str_count_ocurrencies(str, len, '\n');
    utest_assert(count == 2, fail);

    ArlOf(size_t)* ptrs = &(ArlOf(size_t)){0};
    utest_assert(!str_get_lines_offs(str, 0, len, ptrs), fail);
    utest_assert(count == ptrs->len, fail);

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
