#include <stdio.h>

#include <utests.h>

#include <src/error.c>


int test_0(void) {
    Err e0 = err_fmt("FOO");
    utest_assert(strcmp(e0, "FOO") == 0, fail);

    Err e1 = err_fmt("FOO%sBAR", "__");
    utest_assert(strcmp(e1, "FOO__BAR") == 0, fail);

    Err e2 = err_fmt("One%sTwo%sThree", "_1_", "_2_");
    utest_assert(strcmp(e2, "One_1_Two_2_Three") == 0, fail);

    Err e3 = err_fmt("will not be formatted %s", e2);
    Err expt = "error: err_fmt can't receive as parameter an err_fmt return value";
    utest_assert(strcmp(e3, expt) == 0, fail); 


    Err e4 = err_fmt("a number: %d", 713);
    utest_assert(strcmp(e4, "a number: 713") == 0, fail); 

    Err e5 = err_fmt("another number: %d", 1816113);
    utest_assert(strcmp(e5, "another number: 1816113") == 0, fail); 

    Err e6 = err_fmt("a negative number: %d", -816113);
    utest_assert(strcmp(e6, "a negative number: -816113") == 0, fail); 

    Err e7 = err_fmt("another negative number: %d", -7);
    utest_assert(strcmp(e7, "another negative number: -7") == 0, fail); 

    Err e8 = err_fmt("Zero: %d", 0);
    utest_assert(strcmp(e8, "Zero: 0") == 0, fail); 

    Err e9 = err_fmt("a small number: %d", INT_MIN);
    utest_assert(strcmp(e9, "a small number: INT_MIN") == 0, fail); 

    return 0;
fail:
    return -1;
}

int test_1(void) {
    Err e0 = err_fmt(
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
    );
    utest_assert(strcmp(e0,  "error: err_fmt too large.") == 0, fail);

    Err e1 = err_fmt(
        "__FOO__________________________________________________________"
        "__FOO__________________________________________________________"
        "______________________________%s______________________________"
        "__________________________________________________________BAR__",
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
        "()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()"
    );
    utest_assert(strcmp(e1,  "error: err_fmt too large.") == 0, fail);

    Err e2 = err_fmt(
        "_____________________________________________________One  _________________________"
        "_____________________________________________________%s_  _________________________"
        "_____________________________________________________Two  _________________________"
        "_____________________________________________________%s_  _________________________"
        "_____________________________________________________Three_________________________",
        "______________________________________________________________________________1_",
        "_2______________________________________________________________________________"
        "_2______________________________________________________________________________"
    );
    //utest_assert(strcmp(e2, "One_1_Two_2_Three") == 0, fail);
    utest_assert(strcmp(e2,  "error: err_fmt too large.") == 0, fail);


    Err e4 = err_fmt(
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
        "----%d",
        1234567890
    );
    utest_assert(strcmp(e4, "error: not enough size to convert num to str.") == 0, fail); 


    Err e5 = err_fmt(
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
        "---%d",
        -1234567890
    );
    utest_assert(strcmp(e5, "error: not enough size to convert num to str.") == 0, fail); 

    return 0;
fail:
    return -1;
}


int main(void) {
    int errors = test_0()
        + test_1()
        ;
    if (errors) {
        fprintf(stderr, "%d errors\n", errors);
    } else {
        puts("Tests Passed");
    }
}
