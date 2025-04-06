REPO_DIR=..
INCLUDE:=$(HOME)/usr/include
CFLAGS+=-g -std=c2x -Wall -Wextra -Werror -pedantic -Wold-style-definition \
		-I. -I$(REPO_DIR)/che/include -I$(REPO_DIR)/isocline/include

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



