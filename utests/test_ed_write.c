#include <utests.h>
#include <src/cmd-ed.c>


int test_0(void) {
    const char* s1 = "a string with no escape codes";
    const char* res = _mem_find_esc_code_(s1, sizeof s1 - 1);
    utest_assert(res == NULL, fail, __LINE__);

    const char blue[] = EscCodeBlue;
    res = _mem_find_esc_code_(blue, sizeof blue - 1);
    utest_assert(res == blue, fail, __LINE__);

    const char reset[] = EscCodeReset;
    res = _mem_find_esc_code_(reset, sizeof reset - 1);
    utest_assert(res == reset, fail, __LINE__);

    const char s2[] = "0123456789"EscCodeReset;
    res = _mem_find_esc_code_(s2, sizeof s2 - 1);
    utest_assert(res == s2 + 10, fail, __LINE__);
    return 0;
fail:
    return 1;
}

int main(void) {
    print_running_test(__FILE__);
    int errors = test_0();
    print_test_result(errors);
}
