CFLAGS=-Wall -g
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=main-menu

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

demo: $(BIN)
	/usr/share/debconf/frontend ./$(BIN)

# Size optimized and stripped binary target.
small: CFLAGS=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note \
		--strip-unneeded $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
