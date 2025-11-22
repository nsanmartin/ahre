#include <utests.h>

typedef struct { const void* ptr; size_t size; size_t nmemb; } MockFwriteParams;
#define fwrite_params_queue_sz 32
static MockFwriteParams _fwrite_params_queue_[fwrite_params_queue_sz] = {0};
static size_t fwrite_params_queue_right = 0;
static size_t fwrite_call_count = 0;

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
#include "../src/utils.c"
#include "../src/textbuf.c"




void fwrite_params_queue_push(MockFwriteParams expected) {
    if (fwrite_params_queue_right >= fwrite_params_queue_sz) perror(RED"Error "__FILE__"RESET");
    _fwrite_params_queue_[fwrite_params_queue_right++] = expected;
}



static Err _check_write_str_(char* str, size_t len) {
    const char rest[] = "("__FILE__")rest";
    TextBuf* tb = &(TextBuf){.buf={.items=str, .len=len}};
    fwrite_params_queue_push((MockFwriteParams){.ptr=str,.size=1,.nmemb=len});
    /* return _cmd_textbuf_write_impl(&s, tb, &r, rest); */
    return textbuf_to_file(tb, rest, "w");
}

int test_1(void) {
    char items1[] = "12345";
    Err err1 = _check_write_str_(items1, sizeof items1 - 1);
    utest_assert(err1 == Ok, fail);
    utest_assert(fwrite_call_count == 1, fail);

    char items2[] = "123456789";
    Err err2 = _check_write_str_(items2, sizeof items2 - 1);
    utest_assert(err2 == Ok, fail);
    utest_assert(fwrite_call_count == 2, fail);

    char items3[] = "12\045\0789";
    Err err3 = _check_write_str_(items3, sizeof items3 - 1);
    utest_assert(err3 == Ok, fail);
    utest_assert(fwrite_call_count == 3, fail);

    return 0;
fail:
    return 1;
}

int main(void) {
    print_running_test(__FILE__);
    int errors = test_1();
    print_test_result(errors);
}
