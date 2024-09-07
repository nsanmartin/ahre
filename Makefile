INCLUDE:=$(HOME)/usr/include
LIB:=$(HOME)/usr/lib
CFLAGS:=-g  -Wall -Wextra -Werror -pedantic -Wold-style-definition -Iinclude -Ihashi/include

AHRE:=ahre

AHRE_OBJDIR=build
AHRE_SRCDIR=src
AHRE_INCLUDE=include
AHRE_HEADERS=$(wildcard $(AHRE_INCLUDE)/*.h)
AHRE_SRCS=$(wildcard $(AHRE_SRCDIR)/*.c)
AHRE_OBJ=$(AHRE_SRCS:src/%.c=$(AHRE_OBJDIR)/%.o)


ahre-tags: $(AHRE) tags


$(AHRE): $(AHRE_OBJ)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		$(AHRE).c -o build/$(AHRE) \
		$^  \
		-lcurl -llexbor -lreadline


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

test_range: utests/test_range.c $(AHRE_OBJ)
	$(CC) $(CFLAGS) -I$(INCLUDE) -Iutests -o build/$@ $^ \
		-lcurl -llexbor -lreadline


tags:
	ctags -R .

cscope:
	cscope -b -k -R

clean:
	find build -type f -delete
	if [ -f tags ]; then rm tags; fi
