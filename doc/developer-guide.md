# Naming Conventions

# Functions 
All functions and methods are written in snake case.

Functions with internal-linkage are surrounded by underscores, e.g.:
`_convert_to_utf8_`.

Methods (functions tightly associated to types) start with the type name
in snake case (types are in "Pascal" case), e.g.:
`htmldoc_init` (is the init function of `Htmldoc` type).

In implementation files, internal-linkage functions came before 
external-linkage functions after the comment `/* internal linkage */`.
Then came the external-linkage functions after the comment 
`/* external linkage */`.

# Labels
Labels and gotos are used only in functions that reserve and free resources when taking
care of all cases including errors result in easier to follow code in that way. The
simpler example, in which we use the `try` and `try_or_jump` macros, is one like:

```C
Err fn(TypeT out[static 1]) {
    Ptr x1, Ptr x2, Ptr x3, Ptr x4;
    Err e = Ok;
    try ( alloc(x1));
    try_or_jump (e, Clean_X1, alloc(x1));
    try_or_jump (e, Clean_X2, alloc(x1));
    try_or_jump (e, Clean_X3, alloc(x1));
    try_or_jump (e, Clean_X4, alloc(x1));

    e = fn2(x1, x2, x3, x4, out);

Clean_X4: free(x4);
Clean_X3: free(x3);
Clean_X2: free(x2);
Clean_X4: free(x4);
    return e;
}
```

In this example we need to free all reserved memory before returning.

We use _camel-snake-case_ for labels.

# Generation of text representation

Html document is parsed by [lexbor library](https://lexbor.com/).

{#TODO: After parsing the html document,
[quickjs-ng](https://quickjs-ng.github.io/quickjs/)
is used to evaluate the javascript and apply the changes in the document.}

`htmldoc_draw` "draws" the document as text traversing the tree parsed by lexbor. It calls 
`draw_rec_tag` to draw each elemment and for each tag T `draw_tag_T` and `draw_text`r
for text.
