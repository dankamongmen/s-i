include ../../../globalmakeflags

SOBJS=ncurses.so text.so
OBJS=ncurses.o text.o

CFLAGS += -I../..

all: $(SOBJS)

ncurses.so: ncurses.o
	$(CC) $(CFLAGS) -shared -o $@ $^ -lncurses

text.so: text.o
	$(CC) $(CFLAGS) -shared -o $@ $^

clean:
	-rm -f $(SOBJS) $(OBJS) *~
