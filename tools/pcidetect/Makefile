#
#
#

PROG = pcidetect
CC = gcc
CFLAGS = -Wall -Os -fomit-frame-pointer
INSTALL = install
LIBS = -lpci
DESTDIR = 
STRIPTOOL = strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment $(PROG)

all: pcidetect

pcidetect: pcidetect.o
	$(CC) -o pcidetect pcidetect.o $(LIBS) 
	$(STRIP)

pcidetect.o: pcidetect.c lst_net.h lst_net_1000.h lst_net_10_100.h \
		lst_net_arcnet.h lst_net_other.h lst_net_tokenring.h \
		lst_net_wan.h lst_net_wlan.h
	$(CC) $(CFLAGS) -c pcidetect.c

clean:
	-rm -f pcidetect pcidetect.o

install: pcidetect
	$(INSTALL) -d -m 755 $(DESTDIR)/usr/bin
	$(INSTALL) -s -m 755 pcidetect $(DESTDIR)/usr/bin/pcidetect 

