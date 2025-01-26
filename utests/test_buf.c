#include <stdio.h>

#include <utests.h>
#include <src/textbuf.c>



int test_0(void) {

    char* str = "01\n02\n03";
    size_t len = sizeof(str)-1;

    size_t count = mem_count_ocurrencies(str, len, '\n');
    utest_assert(count == 2, fail, __LINE__);

    TextBuf buf;
    textbuf_init(&buf);
    textbuf_append_part(&buf, str, len);
    Err err = textbuf_append_line_indexes(&buf);
    utest_assert(!err, fail, __LINE__);
    utest_assert(count == buf.eols.len, fail, __LINE__);

    textbuf_cleanup(&buf);
    return 0;
fail:
    return 1;
}



int main(void) {
    print_running_test(__FILE__);
    int errors = test_0();
    print_test_result(errors);
}
