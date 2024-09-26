INCLUDE:=$(HOME)/usr/include
LIB:=$(HOME)/usr/lib
CFLAGS:=-g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition -I. -Ihashi/include

AHRE:=ahre

AHRE_OBJDIR=build
AHRE_SRCDIR=src
AHRE_INCLUDE=src
AHRE_HEADERS=$(wildcard $(AHRE_INCLUDE)/*.h)
AHRE_SRCS=$(wildcard $(AHRE_SRCDIR)/*.c)
AHRE_OBJ=$(AHRE_SRCS:src/%.c=$(AHRE_OBJDIR)/%.o)
AHDOC_SRC= debug.c html-doc.c error.c ranges.c session.c str.c textbuf.c user-cmd.c user-interface.c

all: ahre tests

run_tests: tests
	./build/test_buf
	./build/test_range


$(AHRE): $(AHRE_OBJ)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		$@.c -o build/$@ \
		$^  \
		-lcurl -llexbor -lreadline

tests: test_range test_buf

ahdoc: $(AHRE_OBJ)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		$@.c -o build/$@ \
		$^  \
		-lreadline


$(AHRE)2: $(AHRE_OBJ)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		-L$(LIB) \
		$(AHRE).c -o build/$(AHRE) \
		$^  \
		-lcurl -llexbor -lreadline


$(AHRE_OBJDIR)/%.o: $(AHRE_SRCDIR)/%.c $(AHRE_HEADERS)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		-L$(LIB) \
		-c -o $@ \
		$< \
		-lcurl -llexbor

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

test_range: utests/test_range.c build/session.o build/textbuf.o build/url-client.o \
	build/html-doc.o build/str.o build/curl-lxb.o build/lexbor-wrapper.o
	$(CC) $(CFLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor -lreadline

test_buf: utests/test_buf.c build/str.o
	$(CC) $(CFLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor -lreadline

test_error: utests/test_error.c 
	$(CC) $(CFLAGS) -I. -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor -lreadline



tags: $(AHRE_HEADERS) $(AHRE_SRCS)
	ctags -R . 

cscope.files: $(AHRE_HEADERS) $(AHRE_SRCS)
	find . \( -path "./.git"  -o -path "./hashi" \) ! -prune -o -name "*.[ch]" > $@

cscope: cscope.files
	cscope -R -b -i $<


clean:
	find build -type f -delete
	find . -type f -name tags -delete
	find . -type f -name "cscope.*" -delete
	if [ -f tags ]; then rm tags; fi
