#include <utests.h>

#include <stdio.h>
size_t mock_fwrite_ok(const void* ptr, size_t size, size_t nmemb, FILE * stream) {
    (void)ptr;
    (void)stream;
    return size * nmemb;
}

FILE* mock_fopen(const char *restrict pathname, const char *restrict mode) {
    (void)pathname;
    (void)mode;
    return (FILE*)-1;
}

int mock_fclose(FILE *stream) { (void)stream; return 0; }
typedef struct { const void* ptr; size_t size; size_t nmemb; } MockFwriteParams;
#define fwrite_params_queue_sz 32
static MockFwriteParams _fwrite_params_queue_[fwrite_params_queue_sz] = {0};
static size_t fwrite_params_queue_right = 0;
static size_t fwrite_call_count = 0;

void fwrite_params_queue_push(MockFwriteParams expected) {
    if (fwrite_params_queue_right >= fwrite_params_queue_sz) perror(RED"Error "__FILE__"RESET");
    _fwrite_params_queue_[fwrite_params_queue_right++] = expected;
}

size_t mock_fwrite_called_with(const void* ptr, size_t size, size_t nmemb, FILE * stream) {
    (void)stream;
    if (fwrite_call_count >= fwrite_params_queue_right) perror(RED"Error "__FILE__"RESET");
    MockFwriteParams expected = _fwrite_params_queue_[fwrite_call_count++];
    if (ptr == expected.ptr && size == expected.size && nmemb == expected.nmemb) return size * nmemb;
    return 0;
}

#define fopen  mock_fopen
#define fwrite mock_fwrite_called_with
#define fclose mock_fclose
#include "../src/cmd.c"

int test_0(void) {
    const char* s1 = "a string with no escape codes";
    const char* res = _mem_find_esc_code_(s1, sizeof s1 - 1);
    utest_assert(res == NULL, fail);

    const char blue[] = EscCodeBlue;
    res = _mem_find_esc_code_(blue, sizeof blue - 1);
    utest_assert(res == blue, fail);

    const char reset[] = EscCodeReset;
    res = _mem_find_esc_code_(reset, sizeof reset - 1);
    utest_assert(res == reset, fail);

    const char s2[] = "0123456789"EscCodeReset;
    res = _mem_find_esc_code_(s2, sizeof s2 - 1);
    utest_assert(res == s2 + 10, fail);
    return 0;
fail:
    return 1;
}

int test_1(void) {
    const char rest[] = "rest";

    char items1[] = "12345";
    const size_t len1 = sizeof items1 - 1;
    Session s;
    Range r;
    TextBuf* tb1 = &(TextBuf){.buf={.items=items1, .len=len1}};
    fwrite_params_queue_push((MockFwriteParams){.ptr=items1,.size=1,.nmemb=len1});
    Err err1 = _cmd_textbuf_write_impl(&s, tb1, &r, rest);
    utest_assert(err1 == Ok, fail);
    utest_assert(fwrite_call_count == 1, fail);

    char items2[] = "12345"EscCodeYellow"6789";
    const size_t len2 = sizeof items2 - 1;
    TextBuf* tb2 = &(TextBuf){.buf={.items=items2, .len=len2}};
    fwrite_params_queue_push((MockFwriteParams){.ptr=items2,.size=1,.nmemb=5});
    fwrite_params_queue_push(
        (MockFwriteParams){.ptr=items2+5+sizeof EscCodeYellow-1,.size=1,.nmemb=4}
    );
    Err err2 = _cmd_textbuf_write_impl(&s, tb2, &r, rest);
    utest_assert(err2 == Ok, fail);
    utest_assert(fwrite_call_count == 3, fail);

    char items3[] = "12""\0""45"EscCodeYellow"\0""789";
    const size_t len3 = sizeof items3 - 1;
    TextBuf* tb3 = &(TextBuf){.buf={.items=items3, .len=len3}};
    fwrite_params_queue_push((MockFwriteParams){.ptr=items3,.size=1,.nmemb=5});
    fwrite_params_queue_push(
        (MockFwriteParams){.ptr=items3+5+sizeof EscCodeYellow-1,.size=1,.nmemb=4}
    );
    Err err3 = _cmd_textbuf_write_impl(&s, tb3, &r, rest);
    utest_assert(err3 == Ok, fail);
    utest_assert(fwrite_call_count == 5, fail);

    return 0;
fail:
    return 1;
}

int main(void) {
    print_running_test(__FILE__);
    int errors = test_0()
               + test_1();
    print_test_result(errors);
}
