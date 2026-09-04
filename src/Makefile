REPO_DIR = ..

AHRE_CFLAGS = $(CFLAGS) -g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition \
			-Wmissing-prototypes \
			-Wunused-result \
			-Wunused-function \
			-Wunused-label \
			-Wunused-variable \
			-Wunused-value \
			-Wuninitialized \
			-Wshadow \
			-Wformat \
			-Wcast-align \
			-Wnull-dereference \
			-Winit-self
			#-Wconversion


AHRE_CFLAGS += $(shell pkg-config -cflags libcurl)
AHRE_CFLAGS += $(shell pkg-config -cflags lexbor)


LDLIBS += $(shell pkg-config -libs libcurl)
LDLIBS += $(shell pkg-config -libs lexbor)
LDLIBS += -lm


AHRE_OBJDIR       = $(REPO_DIR)/build
AHRE_BUILD        = $(REPO_DIR)/build
AHRE_SRCDIR       = $(REPO_DIR)/src
AHRE_HEADERS      = $(wildcard $(=$(REPO_DIR))/src/*.h)
AHRE_SRCS         = $(wildcard $(AHRE_SRCDIR)/*.c)
AHRE_OBJ          = $(AHRE_SRCS:$(AHRE_SRCDIR)/%.c=$(AHRE_OBJDIR)/%.o)
AHRE_PREPROCESSED = $(AHRE_SRCS:$(AHRE_SRCDIR)/%.c=$(AHRE_OBJDIR)/__pre__%.c)

all: $(AHRE_BUILD)/ahre

AHRE_INCLUDE = -I. -I$(REPO_DIR)/hotl/include -I$(REPO_DIR)/isocline/include
AHRE_DEPS    = $(AHRE_OBJ) $(AHRE_BUILD)/isocline.o

###########################
ifdef AHRE_QUICKJS_DISABLED
###########################

AHRE_CFLAGS += -DAHRE_QUICKJS_DISABLED
AHRE_LDFLAGS = $(LDFLAGS)
AHRE_LDLIBS  = $(LDLIBS)

###########################
else
###########################
ifeq ($(shell pkg-config --exists qjs && echo 1),1) ## qjs exists
AHRE_CFLAGS += $(shell pkg-config -cflags qjs 2> /dev/null)
AHRE_LDLIBS  = $(shell pkg-config -libs qjs 2> /dev/null) $(LDLIBS)
else
QUICKJS_DIR   = $(REPO_DIR)/quickjs
QUICKJS_BUILD = $(QUICKJS_DIR)/build
QUICKJS_LIB   = $(QUICKJS_BUILD)/libqjs.a
AHRE_CFLAGS  += -I$(QUICKJS_DIR)
AHRE_LDFLAGS += -L$(QUICKJS_BUILD)
AHRE_LDLIBS  += -lqjs $(LDLIBS)
AHRE_DEPS	 += $(QUICKJS_LIB)

$(QUICKJS_LIB):
	$(MAKE) -C $(QUICKJS_DIR)


endif

###########################
endif
###########################


$(AHRE_BUILD):
	mkdir -p $@


$(AHRE_BUILD)/ahre: $(AHRE_DEPS)
	$(CC) $(AHRE_CFLAGS) $(AHRE_INCLUDE) \
		$(REPO_DIR)/ahre.c -o $@ \
		$^ \
		$(AHRE_LDFLAGS) $(AHRE_LDLIBS)


$(AHRE_OBJDIR)/%.o: $(AHRE_SRCDIR)/%.c $(AHRE_HEADERS) $(AHRE_BUILD)
	$(CC) $(AHRE_CFLAGS) $(AHRE_INCLUDE) \
		-c -o $@ \
		$< 

$(AHRE_BUILD)/isocline.o:
	$(CC) -c -I$(REPO_DIR)/isocline/include -o $@ $(REPO_DIR)/isocline/src/isocline.c


$(AHRE_BUILD):
	mkdir -p $@

$(AHRE_OBJDIR)/__pre__%.c: $(AHRE_SRCDIR)/%.c
	$(CC) $(AHRE_INCLUDE) -E -o $@ $<


preprocessed: $(AHRE_PREPROCESSED)
	$(CC) $(AHRE_INCLUDE) -E -o $(AHRE_BUILD)/ahre__pre__.c $(REPO_DIR)/ahre.c


tags: $(wildcard $(AHRE_SRCDIR)/*.c)
	ctags -R .

