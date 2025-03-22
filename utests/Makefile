include ../src/Makefile

AHRE_UTESTS=$(REPO_DIR)/utests

run_tests: test_all
	@./build/test_buf
	@./build/test_range
	@./build/test_error
	@./build/test_ed_write

test_all: test_range test_buf test_error test_ed_write

test_range: $(AHRE_UTESTS)/test_range.c $(AHRE_BUILD)/isocline.o \
	$(AHRE_BUILD)/session.o $(AHRE_BUILD)/textbuf.o $(AHRE_BUILD)/url-client.o \
	$(AHRE_BUILD)/htmldoc.o $(AHRE_BUILD)/htmldoc_tag_a.o $(AHRE_BUILD)/str.o \
	$(AHRE_BUILD)/wrapper-lexbor.o $(AHRE_BUILD)/wrapper-lexbor-curl.o $(AHRE_BUILD)/error.o \
	$(AHRE_BUILD)/utils.o $(AHRE_BUILD)/tab_node.o $(AHRE_BUILD)/cmd-ed.o $(AHRE_BUILD)/readpass.o \
	$(AHRE_BUILD)/user-interface.o $(AHRE_BUILD)/user-cmd.o $(AHRE_BUILD)/debug.o \
	$(AHRE_BUILD)/user-out-line-mode.o $(AHRE_BUILD)/user-input.o $(AHRE_BUILD)/user-out-vi-mode.o \
	$(AHRE_BUILD)/tabs.o $(AHRE_BUILD)/bookmark.o 
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor

test_buf: $(AHRE_UTESTS)/test_buf.c $(AHRE_BUILD)/str.o
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) -o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_error: $(AHRE_UTESTS)/test_error.c 
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) -o $(AHRE_BUILD)/$@ $^ \
		-lcurl -llexbor

test_ed_write: $(AHRE_UTESTS)/test_ed_write.c \
	$(AHRE_BUILD)/utils.o $(AHRE_BUILD)/error.o $(AHRE_BUILD)/str.o $(AHRE_BUILD)/range_parse.o $(AHRE_BUILD)/re.o \
	$(AHRE_BUILD)/url-client.o \
	$(AHRE_BUILD)/wrapper-lexbor-curl.o $(AHRE_BUILD)/wrapper-lexbor.o \
	$(AHRE_BUILD)/textbuf.o $(AHRE_BUILD)/session.o $(AHRE_BUILD)/htmldoc.o $(AHRE_BUILD)/htmldoc_tag_a.o \
	$(AHRE_BUILD)/tab_node.o  $(AHRE_BUILD)/readpass.o $(AHRE_BUILD)/isocline.o $(AHRE_BUILD)/user-interface.o \
	$(AHRE_BUILD)/user-cmd.o $(AHRE_BUILD)/debug.o $(AHRE_BUILD)/user-out-line-mode.o $(AHRE_BUILD)/user-input.o \
	$(AHRE_BUILD)/user-out-vi-mode.o $(AHRE_BUILD)/tabs.o $(AHRE_BUILD)/bookmark.o 
	$(CC) $(CFLAGS) $(SANITIZE_FLAGS) -I. -I$(INCLUDE) -I$(AHRE_UTESTS) \
		-o $(AHRE_BUILD)/$@ \
		\
		$^ \
		-lcurl -llexbor



