INCLUDE:=$(HOME)/usr/include
LIB:=$(HOME)/usr/lib
CFLAGS:=-g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition \
		-I. -Ihashi/include -Iisocline/include
SANITIZE_FLAGS:= -fsanitize=leak -fsanitize=address -fsanitize=undefined \
				 -fsanitize=null -fsanitize=bounds -fsanitize=pointer-overflow

AHRE:=ahre

AHRE_OBJDIR=build
AHRE_SRCDIR=src
AHRE_INCLUDE=src
AHRE_HEADERS=$(wildcard $(AHRE_INCLUDE)/*.h)
AHRE_SRCS=$(wildcard $(AHRE_SRCDIR)/*.c)
AHRE_OBJ=$(AHRE_SRCS:src/%.c=$(AHRE_OBJDIR)/%.o)

ADHOC_SRCS := src/error.c src/ranges.c src/str.c src/textbuf.c src/cmd-ed.c src/re.c
ADHOC_OBJ=$(ADHOC_SRCS:src/%.c=$(AHRE_OBJDIR)/%.o)

all: ahre run_tests

run_tests: test_all
	@./build/test_buf
	@./build/test_range
	@./build/test_error
	@./build/test_ed_write

test_all: test_range test_buf test_error test_ed_write

$(AHRE): $(AHRE_OBJ) build/isocline.o
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		$@.c -o build/$@ \
		$^ \
		-lcurl -llexbor


$(AHRE_OBJDIR)/%.o: $(AHRE_SRCDIR)/%.c $(AHRE_HEADERS)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		-L$(LIB) \
		-c -o $@ \
		$< 

build/isocline.o:
	$(CC) -c -Iisocline/include -o $@ isocline/src/isocline.c

test_range: utests/test_range.c \
	build/session.o build/textbuf.o build/url-client.o build/htmldoc.o build/htmldoc_tag_a.o \
	build/str.o build/wrapper-lexbor.o build/wrapper-lexbor-curl.o build/error.o build/utils.o
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor

test_buf: utests/test_buf.c build/str.o
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor

test_error: utests/test_error.c 
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor

test_ed_write: utests/test_ed_write.c \
	build/utils.o build/error.o build/str.o build/ranges.o build/re.o \
	build/url-client.o \
	build/wrapper-lexbor-curl.o build/wrapper-lexbor.o \
	build/textbuf.o build/session.o build/htmldoc.o build/htmldoc_tag_a.o
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor



tags: $(AHRE_HEADERS) $(AHRE_SRCS) clean-tags
	ctags -R . 

cscope.files: $(AHRE_HEADERS) $(AHRE_SRCS)
	if [ -f cscope.files ]; then rm cscope.files; fi
	find . \( -path "./.git"  -o -path "./hashi" \) ! -prune -o -name "*.[ch]" > $@

cscope: cscope.files 
	cscope -R -b -i $<

clean-tags:
	if [ -f tags ]; then rm tags; fi

clean-cscope:
	find . -type f -name "cscope.*" -delete

clean:
	find build -type f -delete


#####################
#TODO: add lexbor as submodule too
debug: curl/lib/.libs 
	$(CC) $(CFLAGS) \
		-I$(INCLUDE)\
		-L./curl/lib/.libs \
		-L$(LIB) \
		$(AHRE).c -o build/$(@) -lcurl -llexbor 


curl/configure:
	cd curl \
		&& libtoolize --force \
		&& aclocal \
		&& autoheader \
		&& automake --force-missing --add-missing \
		&& autoconf \
		&& ./configure  --with-openssl --enable-debug

curl/lib/.libs: curl/configure
	$(MAKE) -C curl

