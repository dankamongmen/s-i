
INCS=-I../cdebconf/src/
LDOPTS=-L../cdebconf/src -ldebconf -lparted
PREFIX=$(DESTDIR)/usr/
CFLAGS=-Wall  -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment


all: partkit

install:
	cp partkit debian/partkit.postinst

partkit: partkit.c
	$(CC) $(CFLAGS) partkit.c  -o $@ $(INCS) $(LDOPTS)
	$(STRIP) $@
	size $@ 

clean:
	rm -f partkit 
