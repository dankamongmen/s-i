ARCH=$(shell dpkg --print-architecture)
CFLAGS=-Wall -g -D_GNU_SOURCE -DARCH=\"$(ARCH)\"
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=anna
LIBS=-ldebconf -ldebian-installer

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

clean:
	-rm -f $(BIN) $(OBJS) *~
