


OBJS=ddetect.o

ifndef PROGS
PROGS=ethdetect
#memdetect diskdetect mdmdetect cddetect cpudetect ethdetect 
endif

HEADERS= cddetect.h mdmdetect.h

INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment 
INCS=-I../cdebconf/src/
LDFLAGS=-L../cdebconf/src/ -ldetect -lisapnp -ldebconf


all: lst2header $(HEADERS) $(OBJS) $(PROGS)
	$(foreach PROG, $(PROGS), \
	($(STRIP) $(PROG)) ; )	

mdmdetect: mdmdetect.c ddetect.o
#	$(CC) mdmdetect.c -o mdmdetect -lisapnp objs/modem.o objs/serial.o objs/utils.o ddetect.o objs/conv.o
cddetect : cddetect.c ddetect.o


ethdetect :ethdetect.c ddetect.o
	$(CC) $< -o $@ $(CFLAGS) $(INCS) $(LDFLAGS)

lst2header: lst2header.c
	$(CC) lst2header.c -o lst2header -ldetect -lisapnp 

cddetect.h: lst2header
	-./lst2header -c > $@ 
mdmdetect.h: lst2header
	-./lst2header -m > $@


clean:
	$(RM) *.o $(PROGS) $(HEADERS) lst2header

install:
	$(foreach PROG, $(PROGS), \
	cp $(PROG) debian/$(PROG).config)
