INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
STRIP = strip --remove-section=.note --remove-section=.comment 
LDFLAGS=-ldebconfclient -lparted
CC=gcc

all: choose-yaboot-disk powerpc-type

choose-yaboot-disk: choose-yaboot-disk.o powerpc-type.o
	$(CC) -o choose-yaboot-disk choose-yaboot-disk.o powerpc-type.o $(LDFLAGS)

choose-yaboot-disk.o: choose-yaboot-disk.c choose-yaboot-disk.h
	$(CC) $(CFLAGS) -c choose-yaboot-disk.c

powerpc-type: powerpc-type.c
	$(CC) -DPOWERPC_TYPE_MAIN -o powerpc-type powerpc-type.c $(LDFLAGS)

powerpc-type.o: powerpc-type.c
	$(CC) $(CFLAGS) -c powerpc-type.c

clean:
	$(RM) choose-yaboot-disk choose-yaboot-disk.o powerpc-type powerpc-type.o

install: all
	install -d $(DESTDIR)/usr/bin
	install -m644 choose-yaboot-disk $(DESTDIR)/usr/bin
	install -m644 powerpc-type $(DESTDIR)/usr/bin

