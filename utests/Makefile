include ../src/Makefile

AHRE_UTESTS=$(REPO_DIR)/utests
SANITIZE_FLAGS:= -fsanitize=leak -fsanitize=address -fsanitize=undefined \
				 -fsanitize=null -fsanitize=bounds -fsanitize=pointer-overflow

run_tests: test_all
	@./build/test_buf \
		&& @./build/test_range \
		&& @./build/test_error \
		&& @./build/test_ed_write \
		@./build/test_iconv

test_all: test_range test_buf test_error test_ed_write test_iconv

test_range: $(AHRE_UTESTS)/test_range.c $(AHRE_BUILD)/isocline.o \
	$(AHRE_BUILD)/session.o $(AHRE_BUILD)/textbuf.o $(AHRE_BUILD)/url-client.o \
	$(AHRE_BUILD)/htmldoc.o $(AHRE_BUILD)/str.o \
	$(AHRE_BUILD)/wrapper-lexbor.o $(AHRE_BUILD)/wrapper-lexbor-curl.o $(AHRE_BUILD)/error.o \
	$(AHRE_BUILD)/utils.o $(AHRE_BUILD)/tab-node.o  $(AHRE_BUILD)/readpass.o \
	$(AHRE_BUILD)/user-interface.o $(AHRE_BUILD)/debug.o \
	$(AHRE_BUILD)/user-input.o $(AHRE_BUILD)/user-out.o \
	$(AHRE_BUILD)/tabs.o $(AHRE_BUILD)/bookmark.o $(AHRE_BUILD)/cmd.o $(AHRE_BUILD)/url.o \
	$(AHRE_BUILD)/js-engine.o \
	$(QUICKJS_LIB)
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor -lm

test_buf: $(AHRE_UTESTS)/test_buf.c $(AHRE_BUILD)/str.o
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) -o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_error: $(AHRE_UTESTS)/test_error.c 
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) -o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_ed_write: $(AHRE_UTESTS)/test_ed_write.c \
	$(AHRE_BUILD)/utils.o $(AHRE_BUILD)/error.o $(AHRE_BUILD)/str.o $(AHRE_BUILD)/range-parse.o \
	$(AHRE_BUILD)/re.o  $(AHRE_BUILD)/url-client.o $(AHRE_BUILD)/url.o \
	$(AHRE_BUILD)/wrapper-lexbor-curl.o $(AHRE_BUILD)/wrapper-lexbor.o \
	$(AHRE_BUILD)/textbuf.o $(AHRE_BUILD)/session.o $(AHRE_BUILD)/htmldoc.o \
	$(AHRE_BUILD)/tab-node.o  $(AHRE_BUILD)/readpass.o $(AHRE_BUILD)/isocline.o $(AHRE_BUILD)/user-interface.o \
	$(AHRE_BUILD)/debug.o $(AHRE_BUILD)/user-out.o $(AHRE_BUILD)/user-input.o \
	$(AHRE_BUILD)/tabs.o $(AHRE_BUILD)/bookmark.o $(AHRE_BUILD)/js-engine.o \
	$(QUICKJS_LIB)

	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor -lm


test_iconv: $(AHRE_UTESTS)/test_iconv.c $(AHRE_BUILD)/error.o
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) -o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_url: $(AHRE_UTESTS)/test_url.c 
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) -o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor
