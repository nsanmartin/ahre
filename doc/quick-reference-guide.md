# Quick reference guide

Ahre can be launched without parameters (`./ahre`) or specifying a url, ie:

```
> ./ahre file:///home/user/file.html
```

If schema is not provided, https is assumed. So 

```
> ./ahre curl.se
```

assumes https://curl.se

In ahre, some tags are wrapped with specific chars:
+ anchors are wrapped within `[]`
+ inputs are wrapped within `{}`
+ images are wrapped within `()`
inside of which a number (integer base 36) identifies the element and its text is
displayed.

This chars are used to indicate specific commands like for example the
comand `[1f*` is used to follow the link with id 1f (see later).

You don't need to type the full command if there is no ambiguity. Hence,
`d` will be interpreted as `draw` (unless we implement a new doc command 
beginning with d).

## Modes

Ahre has different "modes": two line modes (fgets and isocline) and a visual mode (vi). The main
difference is that line modes only print what you ask for while vi mode alway shows something after
each user input. Another difference is that the "session commands" must be preceeded by \ in visual
mode while that's not neeed in line modes. So for example you type:
`\go <URL>`
in visual mode to navigate to <URL> but in text mode yuou may just write 
`go <URL>`.

You change the mode by cli parameter or by setting session configuration inside ahre.

## Help (or ? command)

You get some help and the list of available subcommand by typing <CMD>?, that is teh command itself
followed by the quetion mark ?.

Examples:
+ `\?` explains session commands briefly and lists all session's subcommands
+ `.?` does the same but with . (dot or document command).
+ `{?` 

If you add a `?` on a sub command its help is displayed:

Examples:
+ `\bookmaks?`    the bookmarks help
+ `{=?`           the inputs' set command (used to set the value of a text element in a form).

# Session commands

Start with `\` (but is is optional in line mode and for char commands).
There are two kinds. The word subcommands such as: bookmarks, cookies, set, go, etc. And the char
subcommands: | (tab commands), . (doc commands), : (text buffer commands), < (source buffer commands),
[ (anchor commands), etc.

Type `\?` and `\<SUBCMD>?` to get interactive help. A summary of which follows.

# colon (:) commands 

Similarly to vim, commands starting with colon are buffer commands. After ':'
a range and a command are expected and the current line is set with the end of
the range.

For example: `:write <PATH>` saves current buffer in target (without the escape
sequences bash uses for the colours).


The following apply to line mode:
+ `%p`     prints the whole buffer, sets current line with $.
+ `,+34p`  prints from the current line to 34 lines after it, moves current line 34.
+ `,$n`    print from current line until the last one, numbering the printed
           lines, move the current line.
+ `1p`     prints first line and set the current line.
+ `^`      repeat last range used (for example :^n after :2,5 will be :2,5n).

# less than (<) commands:

These are similar to (:) but operates on the source (html) buffer.

# anchor commands ([)

+ `[<ID>"` shows content of anchor element with id = <ID>
+ `[<ID>*` sets current buffer with anchor's href url fetches and browses it.
+ `[<ID>` same as [<ID>*

# input commands ({)

+ `{<ID>=<VALUE>` sets input with is = <ID> value with <VALUE>. This applies
                  only to text inputs and is used to fill inputs in forms.
+ `{<ID>*` submits button input with id=<ID>. Applied only to submit
                  inputs.
+ `{<ID>"` print information about element (attributes)

# pipe commands (|)

Pipe commands switch between tabs. Tabs have each one its history. It is a tree
where each document is a node. When following a link, a child is appended, when 
navigating back, previous document is set current.

+ `|-` goes back in history
+ `|` displays each tab title with its history
+ `|2` switchs to third tab
+ `|0.1.2.`: switchs to first tab and navigates to third link of second link in history

# dot commands (.)

+ `."` shows info about the document
+ `.+<WORD>` saves current doc into bookmars file under <WORD>
+ `.+/<SUBSTR>`: saves current doc under secction starting with <SUBSTR>


# bookmarks

bookmarks file is an html file the same as w3m's. The idea was to be able to share
the bookmark between both programs. There is an "alias" for the bookmarks file so

go \b

will open it ('\' followed by anu substring of 'bookmarks' will work).

To append the url of a document you can use the dot command:
`.+<SECTION>`

where '+' indicate "add to bookmars" and <SECTION> can be one of:
1. a string starting with '/'
2. a string not starting with '/'

If (1.), for example, .+/foo, a section whose name starts with foo is sought.
Only if it is found it will be appended (for ex. foo or foobar).
If (2.), a full match will be sought. If found it wil be appended to that section,
if not, the section will be created.
