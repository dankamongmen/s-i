OBJS=

ifndef PROGS
PROGS=ethdetect
#memdetect diskdetect mdmdetect cddetect cpudetect ethdetect 
endif


INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment 
LDFLAGS= -ldiscover -ldebconf -ldebian-installer


all: lst2header $(OBJS) $(PROGS)
	$(foreach PROG, $(PROGS), \
	($(STRIP) $(PROG)) ; )	

mdmdetect: mdmdetect.c ddetect.o
#	$(CC) mdmdetect.c -o mdmdetect -lisapnp objs/modem.o objs/serial.o objs/utils.o ddetect.o objs/conv.o
cddetect : cddetect.c ddetect.o

diskdetect : diskdetect.c
	$(CC) $< -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

ethdetect :ethdetect.c
	$(CC) $< -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

lst2header: lst2header.c
	$(CC) lst2header.c -o lst2header -ldiscover

clean:
	$(RM) *.o $(PROGS) lst2header

install:
	$(foreach PROG, $(PROGS), \
	cp $(PROG) debian/$(PROG).postinst)
