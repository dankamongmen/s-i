CFLAGS=-Wall -g
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=main-menu

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

demo: $(BIN)
	/usr/share/debconf/frontend ./$(BIN)

strip: $(BIN)
	strip --remove-section=.comment --remove-section=.note \
		--strip-unneeded $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
