
# define this if you want to link against uC-libc
#UC_LIBC=yes


OBJS=ddetect.o

ifndef PROGS
PROGS=memdetect diskdetect mdmdetect cddetect cpudetect ethdetect 
endif

HEADERS= cddetect.h mdmdetect.h

INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment 
INCS=-I../cdebconf/src/
LDFLAGS=-L../cdebconf/src/ -ldebconf -ldetect -lisapnp 


all: lst2header $(HEADERS) $(OBJS) $(PROGS)
	$(foreach PROG, $(PROGS), \
	($(STRIP) $(PROG)) ; )	

mdmdetect: mdmdetect.c ddetect.o
#	$(CC) mdmdetect.c -o mdmdetect -lisapnp objs/modem.o objs/serial.o objs/utils.o ddetect.o objs/conv.o
cddetect : cddetect.c ddetect.o


ethdetect :ethdetect.c ddetect.o
	$(CC) $< -o $@ $(CFLAGS) $(INCS) $(LDFLAGS) ddetect.o

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
