
LDOPTS=-ldebconf -lparted
PREFIX=$(DESTDIR)/usr/
CFLAGS=-Wall -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment


all: autopartkit

install:
	cp autopartkit debian/autopartkit.postinst

autopartkit: autopartkit.c
	$(CC) $(CFLAGS) autopartkit.c -o $@ $(LDOPTS)
	$(STRIP) $@
	size $@ 

clean:
	rm -f autopartkit 
