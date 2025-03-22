REPO_DIR=..
INCLUDE:=$(HOME)/usr/include
CFLAGS+=-g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition \
		-I. -I$(REPO_DIR)/hashi/include -I$(REPO_DIR)/isocline/include
SANITIZE_FLAGS:= -fsanitize=leak -fsanitize=address -fsanitize=undefined \
				 -fsanitize=null -fsanitize=bounds -fsanitize=pointer-overflow

AHRE_OBJDIR=$(REPO_DIR)/build
AHRE_BUILD=$(REPO_DIR)/build
AHRE_SRCDIR=$(REPO_DIR)/src
AHRE_INCLUDE=$(REPO_DIR)/src
AHRE_HEADERS=$(wildcard $(AHRE_INCLUDE)/*.h)
AHRE_SRCS=$(wildcard $(AHRE_SRCDIR)/*.c)
AHRE_OBJ=$(AHRE_SRCS:$(AHRE_SRCDIR)/%.c=$(AHRE_OBJDIR)/%.o)

all: $(AHRE_BUILD)/ahre

$(AHRE_BUILD)/ahre: $(AHRE_OBJ) $(AHRE_BUILD)/isocline.o
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		$(REPO_DIR)/ahre.c -o $@ \
		$^ \
		-lcurl -llexbor


$(AHRE_OBJDIR)/%.o: $(AHRE_SRCDIR)/%.c $(AHRE_HEADERS)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE) \
		-c -o $@ \
		$< 

$(AHRE_BUILD)/isocline.o:
	$(CC) -c -I$(REPO_DIR)/isocline/include -o $@ $(REPO_DIR)/isocline/src/isocline.c



tags: $(AHRE_HEADERS) $(AHRE_SRCS) clean-tags
	ctags -R --exclude=.git --exclude=hashi/scripts .

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
	find $(AHRE_BUILD) -type f -delete

