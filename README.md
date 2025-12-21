# Ahre

A hypertext reference www navigator or text web browser for linux.

## Website
[ahre web page](https://ahre.pages.dev/)

## Other text browsers:
+ [edbrowse](https://github.com/CMB/edbrowse)
+ [w3m](https://git.sr.ht/~rkta/w3m)


## External dependencies

+ [libcutl](https://curl.se/libcurl/) https://github.com/curl/curl
+ [lexbor](https://lexbor.com/) https://github.com/lexbor/lexbor

## Git submodules:
+ hotl     (repo: https://codeberg.org/nsm/hotl)
+ isocline (repo: https://github.com/daanx/isocline)
+ quickjs  (repo: https://github.com/quickjs-ng/quickjs)

## Build instructions
### Lexbor installation

check [INSTALL.md](https://github.com/lexbor/lexbor/blob/master/INSTALL.md)
in lexbor repo. But basically:
```bash
cd lexbor
cmake .
make
sudo make install
```

### System dependant Dependencies
#### Termux
```bash
pkg install libandroid-wordexp
pkg install libiconv
export LDFLAGS="-landroid-wordexp -liconv"
```

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

#### tcc build
tcc does not support regex and wordexp so currently it won't work.

To build using tcc the `NO_REGEX` flag must be passed:

`CC=tcc CFLAGS=-DAHRE_REGEX_DISABLED make`

To build with no quickjs (and hence no js):
`CFLAGS=-DAHRE_QUICKJS_DISABLED make`

## Run

```
./build/ahre -d 'q=ahre' lite.duckduckgo.com/lite

```

## Usage:
See [./doc/quick-reference-guide.md](https://codeberg.org/nsm/ahre/src/branch/main/doc/quick-reference-guide.md)
