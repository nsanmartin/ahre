#include <stdio.h>

#include <utests.h>

#include "../src/error.c"


int test_0(void) {
    Err e0 = err_fmt("%s", "FOO");
    utest_assert(strcmp(e0, "FOO") == 0, fail);

    Err e1 = err_fmt("FOO%sBAR", "__");
    utest_assert(strcmp(e1, "FOO__BAR") == 0, fail);

    Err e2 = err_fmt("One%sTwo%sThree", "_1_", "_2_");
#define EXPECTED_2 "One_1_Two_2_Three"
    utest_assert(strcmp(e2, EXPECTED_2) == 0, fail);

    const char* f3 = "will it be formatted? %s";
    utest_assert(strcmp(
        _err_fmt_vsnprinf_(f3, e2),
        "error: err_fmt can't receive as parameter an err_fmt return value"
    ) == 0, fail); 
    utest_assert(strcmp(
        _err_fmt_while_(f3, e2),
        "error: err_fmt can't receive as parameter an err_fmt return value"
    ) == 0, fail); 


    const char* f4   = "a number: %d";
    int i4           = 713;
    const char* exp4 =  "a number: 713";
    utest_assert(strcmp(_err_fmt_while_(f4, i4), exp4) == 0, fail); 
    utest_assert(strcmp(_err_fmt_vsnprinf_(f4, i4), exp4) == 0, fail); 


    const char* f5   = "another number: %d";
    int i5           = 1816113;
    const char* exp5 = "another number: 1816113";
    utest_assert(strcmp(_err_fmt_while_(f5, i5), exp5) == 0, fail); 
    utest_assert(strcmp(_err_fmt_vsnprinf_(f5, i5), exp5) == 0, fail); 


    const char* f6   = "a negative number: %d";
    int i6           = -816113;
    const char* exp6 = "a negative number: -816113";
    utest_assert(strcmp(_err_fmt_while_(f6, i6), exp6) == 0, fail); 
    utest_assert(strcmp(_err_fmt_vsnprinf_(f6, i6), exp6) == 0, fail); 


    const char* f7   = "another negative number: %d";
    int i7           = -7;
    const char* exp7 = "another negative number: -7";
    utest_assert(strcmp(_err_fmt_while_(f7, i7), exp7) == 0, fail); 
    utest_assert(strcmp(_err_fmt_vsnprinf_(f7, i7), exp7) == 0, fail); 


    const char* f8   = "Zero: %d";
    int i8           = 0;
    const char* exp8 = "Zero: 0";
    utest_assert(strcmp(_err_fmt_while_(f8, i8), exp8) == 0, fail); 
    utest_assert(strcmp(_err_fmt_vsnprinf_(f8, i8), exp8) == 0, fail); 


    const char* f9   = "a small number: %d";
    int i9           = INT_MIN;
    const char* exp9 = "a small number: INT_MIN";
    utest_assert(strcmp(_err_fmt_while_(f9, i9), exp9) == 0, fail); 
    utest_assert(strcmp(
        _err_fmt_vsnprinf_(f9, -2147483648),
        "a small number: -2147483648"
    ) == 0, fail); 

    return 0;
fail:
    return 1;
}

int test_1(void) {
    const char* s0 = 
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        "FOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOOFOO"
        ;
    utest_assert(strcmp(_err_fmt_while_("%s", s0),  "error: err_fmt too large.") == 0, fail);
    utest_assert(strncmp(_err_fmt_vsnprinf_("%s", s0),  "TRERR:", 6) == 0, fail);

    const char* f1 = 
        "__FOO__________________________________________________________"
        "__FOO__________________________________________________________"
        "______________________________%s______________________________"
        "__________________________________________________________BAR__"
        ;

    const char* s1 = 
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
    ;
    utest_assert(strcmp(_err_fmt_while_(f1, s1),  "error: err_fmt too large.") == 0, fail);
    utest_assert(strncmp(_err_fmt_vsnprinf_(f1, s1),  "TRERR:", 6) == 0, fail);

    const char* f2 = 
        "_____________________________________________________One  _________________________"
        "_____________________________________________________%s_  _________________________"
        "_____________________________________________________Two  _________________________"
        "_____________________________________________________%s_  _________________________"
        "_____________________________________________________Three_________________________"
        ;
    const char* s2_1 = 
        "______________________________________________________________________________1_"
        ;
    const char* s2_2 = 
        "_2______________________________________________________________________________"
        "_3______________________________________________________________________________"
        "_4______________________________________________________________________________"
        "_5______________________________________________________________________________"
        "_6______________________________________________________________________________"
        "_7______________________________________________________________________________"
        "_8______________________________________________________________________________"
    ;
    utest_assert(strcmp(_err_fmt_while_(f2, s2_1, s2_2),  "error: err_fmt too large.") == 0, fail);
    utest_assert(strncmp(_err_fmt_vsnprinf_(f2, s2_1, s2_2),  "TRERR:", 6) == 0, fail);


    const char* f3 = 
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "-------------------%d"
        ;
    int i3 = 1234567890;
    
    utest_assert(
        strcmp(_err_fmt_while_(f3, i3),  "error: not enough size to convert num to str.") == 0,
        fail
    );
    utest_assert(strncmp(_err_fmt_vsnprinf_(f3, i3),  "TRERR:", 6) == 0, fail);


    const char* f4 = 
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "__________________________________________________"
        "-------------------%d"
        ;
    int i4 = -1234567890;

    utest_assert(
        strcmp(_err_fmt_while_(f4, i4),  "error: not enough size to convert num to str.") == 0,
        fail
    );
    utest_assert(strncmp(_err_fmt_vsnprinf_(f4, i4),  "TRERR:", 6) == 0, fail);

    return 0;
fail:
    return 1;
}


int main(void) {
    print_running_test(__FILE__);
    int errors = test_0()
        + test_1()
        ;
    print_test_result(errors);
}
