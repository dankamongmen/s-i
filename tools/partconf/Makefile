LIBS = -lparted -ldl
CFLAGS = -Wall

ifdef DEBUG
CFLAGS += -g3
endif

PARTCONF_OBJS = util.o partconf.o find-parts.o

all: partconf find-partitions mkfstab

partconf: $(PARTCONF_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -ldebconfclient -ldebian-installer

find-partitions: partconf.h find-parts.c util.o
	$(CC) $(CFLAGS) -o $@ -DFIND_PARTS_MAIN find-parts.c util.o $(LIBS) -ldebian-installer

mkfstab: mkfstab.c mkfstab.h
	$(CC) $(CFLAGS) -o $@ mkfstab.c $(LIBS) -ldebian-installer

util.o: util.c
partconf.o: partconf.c partconf.h
find-parts.o: find-parts.c partconf.h

small: CFLAGS += -Os
small: clean partconf find-partitions mkfstab
	strip --remove-section=.comment --remove-section=.note partconf
	strip --remove-section=.comment --remove-section=.note find-partitions
	strip --remove-section=.comment --remove-section=.note mkfstab

clean:
	-rm -f partconf find-partitions mkfstab *.o
