
INCS=-I../cdebconf/src/
LDOPTS=-L../cdebconf/src -ldebconf
PREFIX=$(DESTDIR)/usr/
CFLAGS=-Wall  -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment


all: parted-tiny

install:
	cp parted-tiny debian/parted-tiny.postinst

parted-tiny: parted-tiny.c
	$(CC) $(CFLAGS) parted-tiny.c  -o $@ $(INCS) $(LDOPTS)
	$(STRIP) $@
	size $@ 

clean:
	rm -f parted-tiny 
