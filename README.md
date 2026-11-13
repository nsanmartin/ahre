# Ahre
```
    _/_/_/_/_/_/_/_/_/_/_/_/_/___/
   __/_/_/_/_/_/_/_/_/_/_/_/_/__/
  ___/_/_/_/_/_/_/_/_/_/_/_/___/
 ____/_/_/_/_/_/_/_/_/_/_/____/
__/__/__/___________/__/__/__/
```

A text based web browser for the terminal.

## Website
[ahre web page](https://ahre.pages.dev/)

## Mailing list
[ahre mailing list](https://lists.nongnu.org/mailman/listinfo/ahre)

## IRC channel

[#ahre IRC channel](https://web.libera.chat/#ahre)


## Other text browsers:
+ [lynx](https://github.com/ThomasDickey/lynx-snapshots)
+ [edbrowse](https://github.com/CMB/edbrowse)
+ [w3m](https://git.sr.ht/~rkta/w3m)


## External dependencies

+ [libcurl](https://curl.se/libcurl/) https://github.com/curl/curl
+ [lexbor](https://lexbor.com/) https://github.com/lexbor/lexbor

## Git submodules:
+ hotl     (repo: https://codeberg.org/nsm/hotl)
+ isocline (repo: https://github.com/daanx/isocline)
+ quickjs  (repo: https://github.com/quickjs-ng/quickjs)

## Build instructions
### Lexbor installation

If you package manager does not include lexbor install from sources.

checkout v3.0.0 and follow [INSTALL.md](https://github.com/lexbor/lexbor/blob/master/INSTALL.md)
in lexbor repo. But basically:
```bash
git clone https://github.com/lexbor/lexbor
cd lexbor
git checkout v3.0.0
cmake .
make
sudo make install
```

You may need also to:

```
sudo echo /usr/local/lib > /etc/ls.so.conf.d/lexbor.conf
sudo ldconfig
```

if `liblexbor.so.2 not found` when running `ahre`.

### System dependant Dependencies
#### Termux
```bash
pkg install libandroid-wordexp
pkg install libiconv
export LDFLAGS="-landroid-wordexp -liconv"
export CFLAGS=-fsanitize=address
make AHRE_QUICKJS_DISABLED=1
```

Run `bash/make-termux.sh` if you want tot tpe less.

#### Debian/Ubuntu
```bash
sudo apt install libcurl4-openssl-dev
```
or

```bash
sudo apt install libcurl4-gnutls-dev
```

### Clone Repo

```bash
git clone
git submodule update --init
```

(to update: `git submodule update --recursive --remote`)

### Build
```bash
mkdir build
make ahre
```

To build with no quickjs (and hence no js):
`CFLAGS=-DAHRE_QUICKJS_DISABLED make`

#### tcc build
To build with tcc you need to disable regex passing the `AHRE_REGEX_DISABLED` flag:

`CC=tcc CFLAGS=-DAHRE_REGEX_DISABLED make`

In raspberry pi tcc build fail so qjs should be disabled there:

`CC=tcc CFLAGS="-DAHRE_REGEX_DISABLED -DAHRE_QUICKJS_DISABLED" make`

## Run

```
./build/ahre -d 'q=ahre' lite.duckduckgo.com/lite

```

Search something in duckduckgo
```
ddg  () {
    local IFS="+"
    ahre -d q="$1" lite.duckduckgo.com/lite
}

ddg something
```

## Usage:
See [./doc/quick-reference-guide.md](https://codeberg.org/nsm/ahre/src/branch/main/doc/quick-reference-guide.md)

# Copyright Notice 

Copyright (C) 2026 Nicolas San Martin

# Licence Notice

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without any warranty.
