LIBS = -lparted -ldl
CFLAGS = -Wall

ifdef DEBUG
CFLAGS += -g3
endif

PARTCONF_OBJS = util.o partconf.o find-parts.o

all: partconf find-partitions

partconf: $(PARTCONF_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -ldebconf

find-partitions: partconf.h find-parts.c util.o
	$(CC) $(CFLAGS) -o $@ -DFIND_PARTS_MAIN find-parts.c util.o $(LIBS)

util.o: util.c
partconf.o: partconf.c partconf.h
find-parts.o: find-parts.c partconf.h

small: CFLAGS += -Os
small: clean partconf find-partitions
	strip --remove-section=.comment --remove-section=.note partconf
	strip --remove-section=.comment --remove-section=.note find-partitions

clean:
	-rm -f partconf find-partitions *.o
