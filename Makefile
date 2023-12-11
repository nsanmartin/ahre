CFLAGS:=-g -Wall 

GETHREF=gethref

debug: curl/lib/.libs tidy-html5/build/cmake/libtidy.a
	$(CC) $(CFLAGS) \
		-I./tidy-html5/include \
		-L./curl/lib/.libs \
		-L./tidy-html5/build/cmake \
		$(GETHREF).c -o build/$(GETHREF) -lcurl -ltidy \
		-Wl,-rpath=./tidy-html5/build/cmake

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

tidy-html5/build/cmake/Makefile:
	cd tidy-html5/build/cmake \
		&& cmake ../.. -DCMAKE_BUILD_TYPE=Debug

tidy-html5/build/cmake/libtidy.a: tidy-html5/build/cmake/Makefile
	$(MAKE) -C tidy-html5/build/cmake

tags:
	ctags --languages=c -R .

cscope:
	cscope -b -k -R

clean:
	find build -type f -delete
