CFLAGS=-Wall -W -ggdb -D_GNU_SOURCE
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=anna
LIBS=-ldebconfclient -ldebian-installer

ifdef DEBUG
CFLAGS:=$(CFLAGS) -g3 -DDODEBUG
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

# Size optimized and stripped binary target.
small: CFLAGS:=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

test-anna: anna.c anna.h util.o
	$(CC) -o test-anna -DTEST anna.c util.o $(LIBS)

check: test-anna
	./test-anna

clean:
	-rm -f $(BIN) $(OBJS) *~ test-anna

anna.o util.o: anna.h

.PHONY: check
