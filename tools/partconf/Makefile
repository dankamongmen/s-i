LIBS = -ldebconf -lparted -ldl
CFLAGS = -Wall

ifdef DEBUG
CFLAGS += -g3
endif

OBJS = util.o partconf.o

partconf: $(OBJS)
	$(CC) $(CFLAGS) -o partconf $(OBJS) $(LIBS)

util.o: util.c
partconf.o: partconf.c partconf.h

small: CFLAGS += -Os
small: clean partconf
	strip --remove-section=.comment --remove-section=.note partconf

clean:
	-rm -f partconf *.o
