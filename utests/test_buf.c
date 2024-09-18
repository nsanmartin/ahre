#include <stdio.h>

#include <utests.h>
//#include <ah/buf.h>
#include <src/buf.c>



int test_0(void) {

    char* str = "01\n02\n03";
    size_t len = 8;

    size_t count = str_count_ocurrencies(str, len, '\n');
    utest_assert(count == 2, fail);

    AhBuf buf;
    AhBufInit(&buf);
    int err = AhBufAppendLinesIndexes(&buf, str, len);
    utest_assert(!err, fail);
    utest_assert(count == buf.lines_offs.len, fail);

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
