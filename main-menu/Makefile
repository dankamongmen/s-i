# TODO: the ../../tools/cdebconf bits are temporary, until there is a -dev
# package.
CFLAGS=-Wall -g -D_GNU_SOURCE -I../tools/cdebconf/src
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=main-menu
LIBS=-L../tools/cdebconf/src -ldebconf

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

demo: $(BIN)
	ln -sf debian/templates main-menu.templates
	/usr/share/debconf/frontend ./$(BIN)
	rm -f main-menu.template

# Size optimized and stripped binary target.
small: CFLAGS=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
