include ../../../globalmakeflags

SOBJS=ncurses.so text.so
OBJS=ncurses.opic text.opic

CFLAGS += -I../..

all: $(SOBJS)

ncurses.so: ncurses.opic
	$(CC) $(CFLAGS) -shared -o $@ $^ -lncurses

text.so: text.opic
	$(CC) $(CFLAGS) -shared -o $@ $^

clean:
	-rm -f $(SOBJS) $(OBJS) *~
