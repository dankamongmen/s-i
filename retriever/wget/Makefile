# TODO: the ../../tools/cdebconf bits are temporary, until there is a -dev
# package.
ARCH=$(shell dpkg --print-architecture)
CFLAGS=-Wall -g -D_GNU_SOURCE -I../../tools/cdebconf/src
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=wget-retriever
LIBS=-L../../tools/cdebconf/src -ldebconf

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

# Size optimized and stripped binary target.
small: CFLAGS=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
