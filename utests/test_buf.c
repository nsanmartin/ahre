#include <stdio.h>

#include <utests.h>
#include <src/textbuf.c>



int test_0(void) {

    char* str = "01\n02\n03";
    size_t len = 8;

    size_t count = mem_count_ocurrencies(str, len, '\n');
    utest_assert(count == 2, fail);

    TextBuf buf;
    textbuf_init(&buf);
    ErrStr err = textbuf_append_line_indexes(&buf, str, len);
    utest_assert(!err, fail);
    utest_assert(count == buf.eols.len, fail);

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
