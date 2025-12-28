include ../src/Makefile

AHRE_UTESTS=$(REPO_DIR)/utests
SANITIZE_FLAGS:= -fsanitize=leak,address,undefined,null,bounds,pointer-overflow

run_tests: run_test_ed_write run_test_range run_test_buf run_test_error run_test_iconv run_test_url

run_test_ed_write: test_ed_write
	../build/test_ed_write

run_test_range: test_range
	../build/test_range

run_test_buf: test_buf
	../build/test_buf

run_test_error: test_error
	../build/test_error

run_test_iconv: test_iconv
	../build/test_iconv

run_test_url: test_url
	../build/test_url

test_range: $(AHRE_UTESTS)/test_range.c $(AHRE_BUILD)/isocline.o \
	$(AHRE_BUILD)/session.o $(AHRE_BUILD)/textbuf.o $(AHRE_BUILD)/url-client.o \
	$(AHRE_BUILD)/htmldoc.o $(AHRE_BUILD)/str.o \
	$(AHRE_BUILD)/wrapper-lexbor.o $(AHRE_BUILD)/wrapper-lexbor-curl.o $(AHRE_BUILD)/error.o \
	$(AHRE_BUILD)/utils.o $(AHRE_BUILD)/tab-node.o \
	$(AHRE_BUILD)/readpass.o $(AHRE_BUILD)/reditline.o \
	$(AHRE_BUILD)/user-interface.o $(AHRE_BUILD)/fetch-history.o \
	$(AHRE_BUILD)/user-input.o $(AHRE_BUILD)/user-out.o \
	$(AHRE_BUILD)/tabs.o $(AHRE_BUILD)/bookmark.o $(AHRE_BUILD)/cmd.o $(AHRE_BUILD)/url.o \
	$(AHRE_BUILD)/js-engine.o $(AHRE_BUILD)/wrapper-curl.o \
	$(AHRE_BUILD)/config.o $(AHRE_BUILD)/session-conf.o \
	$(QUICKJS_LIB)
	$(CC) $(AHRE_CFLAGS) $(SANITIZE_FLAGS) $(AHRE_INCLUDE) -I. -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor -lm

test_buf: $(AHRE_UTESTS)/test_buf.c $(AHRE_BUILD)/str.o
	$(CC) $(AHRE_CFLAGS) $(SANITIZE_FLAGS) $(AHRE_INCLUDE) -I. -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_error: $(AHRE_UTESTS)/test_error.c 
	$(CC) $(AHRE_CFLAGS) $(SANITIZE_FLAGS) $(AHRE_INCLUDE) -I. -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_ed_write: $(AHRE_UTESTS)/test_ed_write.c \
	$(AHRE_BUILD)/error.o $(AHRE_BUILD)/str.o $(AHRE_BUILD)/range-parse.o \
	$(AHRE_BUILD)/config.o $(AHRE_BUILD)/re.o  $(AHRE_BUILD)/url-client.o \
	$(AHRE_BUILD)/wrapper-lexbor-curl.o $(AHRE_BUILD)/wrapper-lexbor.o \
	$(AHRE_BUILD)/session.o $(AHRE_BUILD)/htmldoc.o \
	$(AHRE_BUILD)/tab-node.o  $(AHRE_BUILD)/readpass.o $(AHRE_BUILD)/reditline.o \
	$(AHRE_BUILD)/isocline.o $(AHRE_BUILD)/user-interface.o \
	$(AHRE_BUILD)/user-out.o $(AHRE_BUILD)/user-input.o \
	$(AHRE_BUILD)/tabs.o $(AHRE_BUILD)/bookmark.o $(AHRE_BUILD)/js-engine.o \
	$(AHRE_BUILD)/wrapper-curl.o $(AHRE_BUILD)/fetch-history.o \
	$(AHRE_BUILD)/session-conf.o $(AHRE_BUILD)/url.o \
	$(QUICKJS_LIB)

	$(CC) $(AHRE_CFLAGS) $(SANITIZE_FLAGS) $(AHRE_INCLUDE) -I. -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor -lm


test_iconv: $(AHRE_UTESTS)/test_iconv.c $(AHRE_BUILD)/error.o
	$(CC) $(AHRE_CFLAGS) $(SANITIZE_FLAGS) $(AHRE_INCLUDE) -I. -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_url: $(AHRE_UTESTS)/test_url.c \
	$(AHRE_BUILD)/config.o $(AHRE_BUILD)/re.o  $(AHRE_BUILD)/url-client.o $(AHRE_BUILD)/utils.o
	$(CC) $(AHRE_CFLAGS) $(SANITIZE_FLAGS) $(AHRE_INCLUDE) -I. -I$(AHRE_UTESTS) -o \
		$(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor
