#include <utests.h>

#include "../src/utils.h"
#include "../src/user-input.h"
#include "../src/user-input.c"

#define mk_ul_from_lit(Lit) (UserLine){.line=Lit,.len=lit_len__(Lit),.ix=0}
#define mk_ul_from_buf(Buf, Len) (UserLine){.line=Buf,.len=Len,.ix=0}

static int
test_user_line_unskip() {
    UserLine ul = mk_ul_from_lit("");
    user_line_unskip(&ul, 0);
    utest_assert(ul.ix == 0, fail);

    ul = mk_ul_from_lit("01234");
    user_line_unskip(&ul, 0);
    utest_assert(ul.ix == 0, fail);

    ul.ix = 2;
    user_line_unskip(&ul, 1);
    utest_assert(ul.ix == 1, fail);
    user_line_unskip(&ul, 1);
    utest_assert(ul.ix == 0, fail);

    ul.ix = 3;
    user_line_unskip(&ul, 5);
    utest_assert(ul.ix == 0, fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_skip() {
    UserLine ul = mk_ul_from_lit("");
    user_line_skip(&ul, 0);
    utest_assert(ul.ix == 0, fail);
    user_line_skip(&ul, 3);
    utest_assert(ul.ix == 0, fail);

    ul = mk_ul_from_lit("01234");
    user_line_skip(&ul, 0);
    utest_assert(ul.ix == 0, fail);
    user_line_skip(&ul, 3);
    utest_assert(ul.ix == 3, fail);
    user_line_skip(&ul, 3);
    utest_assert(ul.ix == 5, fail);

    ul.ix = 0;
    user_line_skip(&ul, 999);
    utest_assert(ul.ix == 5, fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_skip_space() {
    UserLine ul = mk_ul_from_lit("");
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 0, fail);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 0, fail);

    ul = mk_ul_from_lit("   123");
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);

    user_line_skip(&ul, 3);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 6, fail);

    ul = mk_ul_from_lit("x  123");
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 0, fail);
    user_line_skip(&ul, 3);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);
    user_line_skip_space(&ul);
    utest_assert(ul.ix == 3, fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_match_char() {
    UserLine ul    = mk_ul_from_lit(NULL);
    utest_assert(user_line_match_char(&ul, 'a') == false, fail); 

    ul = mk_ul_from_lit("");
    utest_assert(user_line_match_char(&ul, 'a') == false, fail); 


    ul = mk_ul_from_lit("01234");

    utest_assert(user_line_match_char(&ul, '0'), fail);
    utest_assert(user_line_match_char(&ul, '1'), fail);
    utest_assert(user_line_match_char(&ul, '2'), fail);
    utest_assert(user_line_match_char(&ul, '3'), fail);
    utest_assert(user_line_match_char(&ul, '4'), fail);
    utest_assert(!user_line_match_char(&ul, '5'), fail);

    ul.ix = 0;
    utest_assert(!user_line_match_char(&ul, '1'), fail);
    utest_assert(!user_line_match_char(&ul, '2'), fail);
    utest_assert(!user_line_match_char(&ul, '3'), fail);
    utest_assert(!user_line_match_char(&ul, '4'), fail);
    utest_assert(!user_line_match_char(&ul, '5'), fail);
    utest_assert(user_line_match_char(&ul, '0'), fail);

    ul.ix = 0;
    utest_assert(user_line_match_char(&ul, '0'), fail);
    utest_assert(!user_line_match_char(&ul, '0'), fail);
    utest_assert(!user_line_match_char(&ul, '0'), fail);

    return 0;
fail:
    return 1;
}


static int
test_user_line_match_last_char() {
    CmdOut*   cout = &(CmdOut){0};
    UserLine ul    = mk_ul_from_lit("");
    Err err        = user_line_match_last_char(&ul, 'x', cout);
    utest_assert(err == fail_user_line_no_parse(), fail); 

    ul = mk_ul_from_lit("01234");
    err = user_line_match_last_char(&ul, 'x', cout);
    utest_assert(err == fail_user_line_no_parse(), fail); 

    err = user_line_match_last_char(&ul, '0', cout);
    utest_assert(err == fail_user_line_no_parse(), fail); 

    user_line_skip(&ul, 1);
    err = user_line_match_last_char(&ul, '1', cout);
    utest_assert(err == fail_user_line_no_parse(), fail); 

    user_line_skip(&ul, 3);
    err = user_line_match_last_char(&ul, '4', cout);
    utest_assert(err == Ok, fail); 

    err = user_line_match_last_char(&ul, '4', cout);
    utest_assert(err == fail_user_line_no_parse(), fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_chop_and_skip_space(void) {
    UserLine ul = mk_ul_from_lit(NULL);
    Err err     = user_line_chop_and_skip_space(&ul);
    utest_assert(err == NULL, fail); 

    ul  = mk_ul_from_lit("01234");
    err = user_line_chop_and_skip_space(&ul);
    utest_assert(err != NULL, fail); 

    char buf0[] = { " 1234" } ;
    ul          = mk_ul_from_buf(buf0, 5);
    err         = user_line_chop_and_skip_space(&ul);
    utest_assert(err == NULL, fail); 
    utest_assert(buf0[0] == '\0', fail); 
    err         = user_line_chop_and_skip_space(&ul);
    utest_assert(err != NULL, fail); 

    char buf1[] = { "0   4" } ;
    ul  = mk_ul_from_buf(buf1, 5);
    err = user_line_chop_and_skip_space(&ul);
    utest_assert(err != NULL, fail); 

    user_line_skip(&ul, 1);
    err = user_line_chop_and_skip_space(&ul);
    utest_assert(err == NULL, fail); 
    utest_assert(user_line_match_char(&ul, '4'), fail);
    utest_assert(buf1[1] == '\0', fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_cmd_end(void) {
    UserLine ul = mk_ul_from_lit(NULL);
    utest_assert(user_line_cmd_end(&ul), fail); 

    ul  = mk_ul_from_lit("01234");
    utest_assert(!user_line_cmd_end(&ul), fail); 

    ul  = mk_ul_from_lit("0;234");
    utest_assert(!user_line_cmd_end(&ul), fail); 
    ul.ix = 1;
    utest_assert(user_line_cmd_end(&ul), fail); 

    ul  = mk_ul_from_lit("0>234");
    utest_assert(!user_line_cmd_end(&ul), fail); 
    ul.ix = 1;
    utest_assert(user_line_cmd_end(&ul), fail); 

    ul  = mk_ul_from_lit("0\0""234");
    utest_assert(!user_line_cmd_end(&ul), fail); 
    ul.ix = 1;
    utest_assert(user_line_cmd_end(&ul), fail); 

    ul  = mk_ul_from_lit("    ");
    utest_assert(!user_line_cmd_end(&ul), fail); 
    return 0;
fail:
    return 1;
}



static int
test_user_line_cmd_end_skipping_space(void) {
    UserLine ul = mk_ul_from_lit(NULL);
    utest_assert(user_line_cmd_end_skipping_space(&ul), fail); 

    ul  = mk_ul_from_lit("    ");
    utest_assert(user_line_cmd_end_skipping_space(&ul), fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_cut_cmd(void) {
    CmdOut*   cout = &(CmdOut){0};
    UserLine ul    = mk_ul_from_lit("");
    utest_assert(user_line_cut_cmd(&ul, cout) == Ok, fail); 

    ul = mk_ul_from_lit("0123");
    ul.ix = 2;
    utest_assert(user_line_cut_cmd(&ul, cout) == fail_user_line_no_parse(), fail); 

    ul = mk_ul_from_lit("  23");
    ul.ix = 2;
    utest_assert(user_line_cut_cmd(&ul, cout) == fail_user_line_no_parse(), fail); 

    ul = mk_ul_from_lit("  23");
    ul.ix = 2;
    utest_assert(user_line_cut_cmd(&ul, cout) == fail_user_line_no_parse(), fail); 

    ul = mk_ul_from_lit("     1233");
    utest_assert(user_line_cut_cmd(&ul, cout) == fail_user_line_no_parse(), fail); 

    char buf0[] = { "  ;456" } ;
    ul = mk_ul_from_buf(buf0, 6);
    utest_assert(user_line_cut_cmd(&ul, cout) == Ok, fail); 
    utest_assert(buf0[3] == '\0', fail); 

    char buf1[] = { "  > msg" } ;
    ul = mk_ul_from_buf(buf1, 7);
    utest_assert(user_line_cut_cmd(&ul, cout) == Ok, fail); 
    utest_assert(buf1[3] == '\0', fail); 

    char buf2[] = { "  > not a sink" } ;
    ul = mk_ul_from_buf(buf2, 14);
    utest_assert(user_line_cut_cmd(&ul, cout) != Ok, fail); 

    char buf3[] = { "  > std < msg " } ;
    ul = mk_ul_from_buf(buf3, 14);
    utest_assert(user_line_cut_cmd(&ul, cout) == Ok, fail); 
    utest_assert(buf3[3] == '\0', fail); 

    char buff4[] = { "  > std < not src " } ;
    ul = mk_ul_from_buf(buff4, 14);
    utest_assert(user_line_cut_cmd(&ul, cout) != Ok, fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_parse_range(void) {
    RangeParse rp = (RangeParse){0};
    UserLine ul   = mk_ul_from_lit("");
    Err err       = Ok;
    
    err = user_line_parse_range(&ul, 10, &rp);
    utest_assert(err == Ok, fail); 

    ul = mk_ul_from_lit("not a range");
    err = user_line_parse_range(&ul, 10, &rp);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 0, fail); 

    ul = mk_ul_from_lit("1");
    err = user_line_parse_range(&ul, 10, &rp);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 1, fail); 

    ul = mk_ul_from_lit("19");
    err = user_line_parse_range(&ul, 10, &rp);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 2, fail); 

    ul = mk_ul_from_lit("1a");
    err = user_line_parse_range(&ul, 10, &rp);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 1, fail); 

    ul = mk_ul_from_lit("1,1");
    err = user_line_parse_range(&ul, 10, &rp);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 3, fail); 
    return 0;
fail:
    return 1;
}



static int
test_user_line_parse_size_t_or_throw(void) {
    Err err       = Ok;
    UserLine ul   = mk_ul_from_lit("");
    size_t num;

    err = user_line_parse_size_t_or_throw(&ul, &num, 10);
    utest_assert(err != Ok, fail); 

    ul = mk_ul_from_lit("a");
    err = user_line_parse_size_t_or_throw(&ul, &num, 10);
    utest_assert(err != Ok, fail); 

    ul = mk_ul_from_lit("1");
    err = user_line_parse_size_t_or_throw(&ul, &num, 10);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 1, fail); 

    ul = mk_ul_from_lit("1a");
    err = user_line_parse_size_t_or_throw(&ul, &num, 10);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 1, fail); 


    ul = mk_ul_from_lit("9999 ");
    err = user_line_parse_size_t_or_throw(&ul, &num, 10);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 4, fail); 

    ul = mk_ul_from_lit("99999999999999999999999 ");
    err = user_line_parse_size_t_or_throw(&ul, &num, 10);
    utest_assert(err != Ok, fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_char(void) {
    Err err     = Ok;
    UserLine ul = mk_ul_from_lit("");
    char ch;

    err = user_line_char(&ul, &ch);
    utest_assert(err != Ok, fail); 

    ul  = mk_ul_from_lit("0123");
    err = user_line_char(&ul, &ch);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 0, fail); 
    utest_assert(ch == '0', fail); 

    ++ul.ix;
    err = user_line_char(&ul, &ch);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 1, fail); 
    utest_assert(ch == '1', fail); 

    ++ul.ix;
    err = user_line_char(&ul, &ch);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 2, fail); 
    utest_assert(ch == '2', fail); 

    ++ul.ix;
    err = user_line_char(&ul, &ch);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 3, fail); 
    utest_assert(ch == '3', fail); 

    ++ul.ix;
    err = user_line_char(&ul, &ch);
    utest_assert(err != Ok, fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_pop_strview(void) {
    UserLine ul   = mk_ul_from_lit("");
    StrView view;
    bool res;

    res = user_line_pop_strview(&ul, strview_split_alpha, &view);
    utest_assert(res == false, fail); 
    utest_assert(ul.ix == 0, fail); 

    ul  = mk_ul_from_lit("0123");
    res = user_line_pop_strview(&ul, strview_split_alpha, &view);
    utest_assert(res == false, fail); 
    utest_assert(ul.ix == 0, fail); 

    ul  = mk_ul_from_lit("abcd");
    res = user_line_pop_strview(&ul, strview_split_alpha, &view);
    utest_assert(res == true, fail); 
    utest_assert(ul.ix == 4, fail); 

    ul  = mk_ul_from_lit("1bcd");
    res = user_line_pop_strview(&ul, strview_split_alpha, &view);
    utest_assert(res == false, fail); 
    utest_assert(ul.ix == 0, fail); 

    ul  = mk_ul_from_lit("abcd2");
    res = user_line_pop_strview(&ul, strview_split_alpha, &view);
    utest_assert(res == true, fail); 
    utest_assert(ul.ix == 4, fail); 

    return 0;
fail:
    return 1;
}


static int
test_user_line_pop_last_strview(void) {
    CmdOut*  cout = &(CmdOut){0};
    UserLine ul   = mk_ul_from_lit("");
    Err err       = Ok;
    StrView view;

    err = user_line_pop_last_strview(&ul, strview_split_alpha, &view, cout);
    utest_assert(err != Ok, fail); 

    ul  = mk_ul_from_lit("0123");
    err = user_line_pop_last_strview(&ul, strview_split_alpha, &view, cout);
    utest_assert(err != Ok, fail); 

    ul  = mk_ul_from_lit("abcd");
    err = user_line_pop_last_strview(&ul, strview_split_alpha, &view, cout);
    utest_assert(err == Ok, fail); 
    utest_assert(ul.ix == 4, fail); 

    ul  = mk_ul_from_lit("1bcd");
    err = user_line_pop_last_strview(&ul, strview_split_alpha, &view, cout);
    utest_assert(err != Ok, fail); 

    ul  = mk_ul_from_lit("abcd2");
    err = user_line_pop_last_strview(&ul, strview_split_alpha, &view, cout);
    utest_assert(err == fail_user_line_no_parse(), fail); 
    utest_assert(ul.ix == 0, fail); 

    return 0;
fail:
    return 1;
}




int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_user_line_match_char()
        + test_user_line_unskip()
        + test_user_line_skip()
        + test_user_line_skip_space()
        + test_user_line_match_last_char()
        + test_user_line_chop_and_skip_space()
        + test_user_line_cmd_end()
        + test_user_line_cmd_end_skipping_space()
        + test_user_line_cut_cmd()
        + test_user_line_parse_range()
        + test_user_line_parse_size_t_or_throw()
        + test_user_line_char()
        + test_user_line_pop_strview()
        + test_user_line_pop_last_strview()
        ;
    print_test_result(errors);
}

