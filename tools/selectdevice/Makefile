INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
STRIP = strip --remove-section=.note --remove-section=.comment 
LDFLAGS=-lkdetect -ldebconf
CC=gcc

all: selectdevice

selectdevice: selectdevice.o
	$(CC) -o selectdevice selectdevice.o $(LDFLAGS)

selectdevice.o: selectdevice.c
	$(CC) $(CFLAGS) -c selectdevice.c

clean:
	$(RM) selectdevice selectdevice.o 

install: all
	install -d $(DESTDIR)/usr/bin
	install -m644 selectdevice $(DESTDIR)/usr/bin

