CFLAGS=-Wall -g
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=main-menu

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

demo: $(BIN)
	/usr/share/debconf/frontend ./$(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~
