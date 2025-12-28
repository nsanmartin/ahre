REPO_DIR=..
QUICKJS_REPO=$(REPO_DIR)/quickjs
QUICKJS_INCLUDE=$(QUICKJS_REPO)
QUICKJS_BUILD=$(QUICKJS_REPO)/build
QUICKJS_LIB=$(QUICKJS_BUILD)/libqjs.a

AHRE_CFLAGS=$(CFLAGS) -g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition \
			-Wunused-result -Wuninitialized -Wshadow
AHRE_INCLUDE=-I. -I$(REPO_DIR)/hotl/include -I$(REPO_DIR)/isocline/include -I$(QUICKJS_INCLUDE)

AHRE_LDFLAGS=$(LDFLAGS) -lcurl -llexbor -lm

AHRE_OBJDIR=$(REPO_DIR)/build
AHRE_BUILD=$(REPO_DIR)/build
AHRE_SRCDIR=$(REPO_DIR)/src
AHRE_HEADERS=$(wildcard $(=$(REPO_DIR))/src/*.h)
AHRE_SRCS=$(wildcard $(AHRE_SRCDIR)/*.c)
AHRE_OBJ=$(AHRE_SRCS:$(AHRE_SRCDIR)/%.c=$(AHRE_OBJDIR)/%.o)

all: $(AHRE_BUILD)/ahre

$(AHRE_BUILD)/ahre: $(AHRE_OBJ) $(AHRE_BUILD)/isocline.o $(QUICKJS_LIB)
	$(CC) $(AHRE_CFLAGS) $(AHRE_INCLUDE) \
		$(REPO_DIR)/ahre.c -o $@ \
		$^ \
		-L$(QUICKJS_BUILD) \
		$(AHRE_LDFLAGS) $(LDFLAGS)


$(AHRE_OBJDIR)/%.o: $(AHRE_SRCDIR)/%.c $(AHRE_HEADERS)
	$(CC) $(AHRE_CFLAGS) $(AHRE_INCLUDE) \
		-c -o $@ \
		$< 

$(AHRE_BUILD)/isocline.o:
	$(CC) -c -I$(REPO_DIR)/isocline/include -o $@ $(REPO_DIR)/isocline/src/isocline.c



$(QUICKJS_LIB):
	$(MAKE) -C $(QUICKJS_REPO)
