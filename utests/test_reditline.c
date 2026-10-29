#include <stdio.h>

#include <utests.h>

static inline int mock_putchar(char c) { return (unsigned char)c; }
#define putchar mock_putchar
#define fwrite mock_fwrite_ok

#include "../src/reditline.c"

#define rlbuf_clean_(RLB)  rl_cleanup(&(ReditLine){.buf=RLB})


static int
test_rl_init(void) {
    ReditLine rl;
    RlError lrerr = rl_init(&rl, NULL, NULL);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.capacity == RL_DEFAULT_LINE_CAPACITY, Fail);

    rl_cleanup(&rl);
    return 0;
Fail:
    rl_cleanup(&rl);
    return 1;
}

static int
test_rlbuf_ensure_extra_capacity(void) {
    RLBuf rlb = (RLBuf){0};

    rlbuf_ensure_extra_capacity_(&rlb, 0);
    utest_assert(rlb.len == 0, Fail);
    utest_assert(rlb.capacity == 0, Fail);
    utest_assert(rlb.pos == 0, Fail);

    rlbuf_ensure_extra_capacity_(&rlb, 3);
    utest_assert(rlb.len == 0, Fail);
    utest_assert(rlb.capacity >= 3, Fail);
    utest_assert(rlb.pos == 0, Fail);


    rlbuf_ensure_extra_capacity_(&rlb, 12462);
    utest_assert(rlb.len == 0, Fail);
    utest_assert(rlb.capacity >= 3 + 12462, Fail);
    utest_assert(rlb.pos == 0, Fail);

    rlbuf_clean_(rlb);
    return 0;
Fail:
    rlbuf_clean_(rlb);
    return 1;
}



static int
test_rl_insert_char(void) {
    ReditLine rl;
    RlError lrerr = rl_init(&rl, NULL, NULL);
    utest_assert(lrerr == ReditlineOk, Fail);

    /* Three times init capacity so realloc should be called */
    size_t N = 3 * RL_DEFAULT_LINE_CAPACITY; 
    for (size_t i = 0; i < N; ++i) {
        lrerr = rl_insert_char(&rl, '0');
        utest_assert(rl.buf.pos == i+1 , Fail);
        utest_assert(lrerr == ReditlineOk, Fail);
    }

    utest_assert(rl.buf.pos >= N , Fail);
    utest_assert(rl.buf.capacity >= N , Fail);
    utest_assert(rl.buf.len == N , Fail);
    rl_cleanup(&rl);
    return 0;
Fail:
    rl_cleanup(&rl);
    return 1;
}


static int
test_rl_delete_char_forward(void) {
    ReditLine rl;
    RlError lrerr = rl_init(&rl, NULL, NULL);
    utest_assert(lrerr == ReditlineOk, Fail);

    lrerr = rl_delete_char_forward(&rl);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.pos == 0 , Fail);

    lrerr = rl_insert_char(&rl, '0');
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(rl.buf.len == 1 , Fail); /* do not delete nothing */
    utest_assert(rl.buf.pos == 1 , Fail);
    utest_assert(lrerr == ReditlineOk, Fail);

    rl.buf.pos = 0;
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(rl.buf.len == 0 , Fail); /* deleting */
    utest_assert(rl.buf.pos == 0 , Fail);
    utest_assert(lrerr == ReditlineOk, Fail);

    lrerr = rl_insert_char(&rl, '1');
    lrerr = rl_insert_char(&rl, '2');
    lrerr = rl_insert_char(&rl, '3');
    lrerr = rl_insert_char(&rl, '4');
    utest_assert(rl.buf.len == 4 , Fail);
    utest_assert(memcmp(rl.buf.items, "1234", 4) == 0, Fail);

    rl.buf.pos = 0;
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.pos == 0 , Fail);
    utest_assert(rl.buf.len == 3 , Fail);
    utest_assert(memcmp(rl.buf.items, "234", 3) == 0, Fail);
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.pos == 0 , Fail);
    utest_assert(rl.buf.len == 2 , Fail);
    utest_assert(memcmp(rl.buf.items, "34", 2) == 0, Fail);
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.pos == 0 , Fail);
    utest_assert(rl.buf.len == 1 , Fail);
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.pos == 0 , Fail);
    utest_assert(rl.buf.len == 0 , Fail);
    lrerr = rl_delete_char_forward(&rl);
    utest_assert(lrerr == ReditlineOk, Fail);
    utest_assert(rl.buf.pos == 0 , Fail);

    rl_cleanup(&rl);
    return 0;
Fail:
    rl_cleanup(&rl);
    return 1;
}


static int
test_rl_move_cursor(void) {
// #define EscCodeDirectionUp          'A'
// #define EscCodeDirectionDown        'B'
// #define EscCodeDirectionForward     'C'
// #define EscCodeDirectionBackward    'D'

    RlError rlerr = rl_move_cursor(1, EscCodeDirectionUp);
    utest_assert(rlerr == ReditlineOk, Fail);

    rlerr = rl_move_cursor(1, '0');
    utest_assert(rlerr == RlErrorInvalidInput, Fail);

    rlerr = rl_move_cursor(9999999, EscCodeDirectionUp);
    utest_assert(rlerr == RlErrorInvalidInput, Fail);
    return 0;
Fail:
    return 1;
}


int main(void) {
    print_running_test(__FILE__);
    int errors = test_rl_init() 
               + test_rlbuf_ensure_extra_capacity()
               + test_rl_insert_char()
               + test_rl_delete_char_forward()
               + test_rl_move_cursor()
               ;
    print_test_result(errors);
}
