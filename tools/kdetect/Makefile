DESTDIR=
CFLAGS=-Wall -Os -fomit-frame-pointer
LDFLAGS=
CC=gcc

all: libkdetect.so

libkdetect.so: libkdetect.o common.o proc_devices.o proc_ide.o proc_mdstat.o
	$(CC) -fPIC -shared -Wl,-soname,libkdetect.so.0 -o libkdetect.so.0.1 libkdetect.o common.o proc_devices.o proc_ide.o proc_mdstat.o

libkdetect.o: libkdetect.c
	$(CC) $(CFLAGS) -c libkdetect.c

common.o: common.c
	$(CC) $(CFLAGS) -c common.c

proc_devices.o: proc_devices.c common.c
	$(CC) $(CFLAGS) -c proc_devices.c 

proc_ide.o: proc_ide.c common.c
	$(CC) $(CFLAGS) -c proc_ide.c 

proc_mdstat.o: proc_mdstat.c common.c
	$(CC) $(CFLAGS) -c proc_mdstat.c

clean:
	rm -f libkdetect.so.0.1 libkdetect.a kdetect *.o 

install-bin: libkdetect.so
	install -d $(DESTDIR)/usr/lib
	install libkdetect.so.0.1 $(DESTDIR)/usr/lib
	install -m644 libkdetect.so.0.1 $(DESTDIR)/usr/lib

install-dev: 
	install -d $(DESTDIR)/usr/include
	install libkdetect.h $(DESTDIR)/usr/include
	install -m644 libkdetect.h $(DESTDIR)/usr/include

install: install-bin install-dev
