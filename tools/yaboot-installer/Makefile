INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
STRIP = strip --remove-section=.note --remove-section=.comment 
LDFLAGS=-lkdetect -ldebconf -lparted
CC=gcc

all: choose-yaboot-disk powerpc-type

choose-yaboot-disk: choose-yaboot-disk.o
	$(CC) -o choose-yaboot-disk choose-yaboot-disk.o $(LDFLAGS)

choose-yaboot-disk.o: choose-yaboot-disk.c
	$(CC) $(CFLAGS) -c choose-yaboot-disk.c

clean:
	$(RM) choose-yaboot-disk choose-yaboot-disk.o powerpc-type powerpc-type.o

install: all
	install -d $(DESTDIR)/usr/bin
	install -m644 choose-yaboot-disk $(DESTDIR)/usr/bin
	install -m644 powerpc-type $(DESTDIR)/usr/bin

