CFLAGS=-Wall -W -g -D_GNU_SOURCE
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=main-menu
LIBS=-ldebconfclient -ldebian-installer

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG=1
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

demo: $(BIN)
	ln -sf debian/templates main-menu.templates
	/usr/share/debconf/frontend ./$(BIN)
	rm -f main-menu.template

# Size optimized and stripped binary target.
small: CFLAGS:=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
