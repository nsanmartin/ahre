# Testing
## Manual testing

An easy way to find bugs, mainly memory leaks and invalid reads, is using valgring.
Just run
```
$ valgrind --leak-check=full  --track-origins=yes --show-leak-kinds=all  ahre 2> log
```
and use it to navigate until something is logged.

If you want to find bugs related to error management, you can build with:

```
$ make clean
$ CFLAGS=-DAHRE_SIMULATE_ERR=5000 make
```

passing whatever number you want. This will simulate different kinds of errors
every N times and allows to test the error management (use after free, mempry
leak, etc.)

## Unit tests
There are very few unit tests and this should be fixed.

There is a script `bash/check-utests` that run all unit tests with both gcc and
clang. tcc build is broken due to a dependency, but we should renable it (at
list failing when that dependency is attempted to be called).

# Errors
In C, errors are indicated in various ways:

`malloc`, `calloc`, `realloc`, etc. return `NULL`.

`fwrite` returns a short number and `fread` requires to check with `ferror` and `feof`.

`iconv_open` returns `-1` and sets the global variable `errno`, so does `mkdir`.

`realpath` returns `NULL` and also sets `errno`.

libcurl functions use codes such as `CURLcode` and `CURLUcode`.

lexbor functions use `lexbor_status_t`.

In quickjs functions returns ths `JSValue` `JS_EXCEPTION`.

Given all this variety what we use it wrap every function so that we can only use functions
that either return `Err` or does not have errors (like `isspace`).


This allows us to use the `try` macro:
```
#define try(Expr) _Generic((Expr), Err: do{\
    Err ahre_err_= (Expr);\
    if (ahre_err_) return ahre_err_;\
} while(0) 
```

It first checks the parameter type, so that the compilers fails if we pass any
other type. It uses the `do{}while(0)` trick that creates a scope for the local
variable so that it does not clash with any other `try` call. It assigns `Expr`
to it so that it is evaluated exactly once. Finally, it "early" returns if an
error is in fact received, so that it works just like the `||` operator for
booleans.

In summary, our `try` macro is similar to zig's try and you can write functions like:

```
Err f(T v[1], U out[1]) {
    try( g0(v, out) );
    try( g1(v, out) );
    ...
    try( gn(v, out) );
    return Ok;
}
```
Only if there are no errors, `f` will return `Ok`, if not, as soon as
an error is received it is passed to the caller.

All this works fine as long as no resources are involved. But what happen if, say, `g1`
end calling `malloc` and we lost the pointer to that memory?

Well, for that cases we use `try_or_jump`:

```

#define try_or_jump(ErrLval, Label, Expr) _Generic((Expr), Err: do{\
    ErrLval=(Expr);
    if (ErrLval) goto Label;
} while(0)
```

which is similar to `try` but instead of returning, it jumps to a tag we use to do any 
required cleanup.

For example, consider this `TypeT` "constructor":
```
Err type_t_init(TypeT out[1]) {
    T1 x1 = (T1){0};
    T2 x2 = (T2){0};
    T3 x3 = (T3){0};
    T4 x4 = (T4){0};
    Err err = Ok;

    try ( init1(x1));
    try_or_jump (err, Clean, init2((x2));
    try_or_jump (err, Clean, init3((x3));
    try_or_jump (err, Clean, init4((x4));
    try_or_jump (err, Clean, init5((x5));
    try_or_jump(e, Clean, fn2(x1, x2, x3, x4, out));

    return Ok;

Clean:
    clean1(x1);
    clean2(x2);
    clean3(x3);
    clean4(x4);
    return err;
}
```

On success, `out` owns all resources attached to `x1`,..., `x4`, and it is `type_t_init`'s caller 
responsibility to call `type_t_clean` to cleanup everything.

On error, all is cleaned after the `Clean` tag. We are assuming here that we can call the destructor 
on an object not yet initialized (that is how we implement the destructor) and that any function 
will never reserve resources if it returns an error.

# Naming Conventions

## Types
Types are in "Pascal" case such as in `HtmlDoc`, `JsEngine`, `Session`, etc.

## Functions 
All functions and methods are written in snake case.

Methods (functions tightly associated to types) start with the type name
in snake case , e.g.: `htmldoc_init` (is the init function of `Htmldoc` type).

Header files should not have implementations and only functions defined 
with `external linkage` intead (you may found exceptions, ie `static inline`
functions, but those should be corrected).

All functions that are not exposed in an interface (a .c file) are `static`, ie, 
have `internal linkage`.

In function signatures, we differentiate between `Type* p` and `Type p{_1_]`, 
where `_1_` is just a macro for `1`. The first form is only used when we may
receive `NULL`, the second when we don;t expect it.

## Labels
Labels are used together with the `try_or_jump` macro and only in functions
that reserve and free resources when taking care of all cases including errors
result in easier to follow code in that way. 

We use _camel-snake-case_ for labels.

# Generation of textual representation of the html.

Html document is parsed by [lexbor library](https://lexbor.com/).

{#TODO: After parsing the html document,
[quickjs-ng](https://quickjs-ng.github.io/quickjs/)
is used to evaluate the javascript and apply the changes in the document.}

`htmldoc_draw` "draws" the document as text traversing the tree parsed by lexbor. It calls 
`draw_rec_tag` to draw each element and for each tag T `draw_tag_T` and `draw_text`r
for text.

# Debug Build

The default build `make ahre` uses the `-g` flag so can be used for debugging. There is
an additional build `make -f Makefile.debug` in `./src` that uses the
`$HOME/test/local/lib` to link libcurl. So to be able to step into libcurl code
you need to build curl configuring it with
`./confugure --prefix=$HOME/test/local --with-openssl --enble-debug`

or with any other ssl option.

And for quickjs, you need to `make debug` the quickjs sources.

The target is `./build/debug`.

# Adding a module
When creating a module, the utest's Makefile should be updated to include it as dependency 
if required. This does not apply to all cases though.
