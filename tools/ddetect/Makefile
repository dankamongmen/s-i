


OBJS=utils.o

ifndef PROGS
PROGS=ethdetect
#memdetect diskdetect mdmdetect cddetect cpudetect ethdetect 
endif


INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer 
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment 
INCS=-I../cdebconf/src/
LDFLAGS=-L../cdebconf/src/ -ldetect -lisapnp -ldebconf


all: lst2header $(OBJS) $(PROGS)
	$(foreach PROG, $(PROGS), \
	($(STRIP) $(PROG)) ; )	

mdmdetect: mdmdetect.c ddetect.o
#	$(CC) mdmdetect.c -o mdmdetect -lisapnp objs/modem.o objs/serial.o objs/utils.o ddetect.o objs/conv.o
cddetect : cddetect.c ddetect.o


ethdetect :ethdetect.c
	$(CC) $< -o $@ $(OBJS) $(CFLAGS) $(INCS) $(LDFLAGS)

lst2header: lst2header.c
	$(CC) lst2header.c -o lst2header -ldetect -lisapnp 

clean:
	$(RM) *.o $(PROGS) lst2header

install:
	$(foreach PROG, $(PROGS), \
	cp $(PROG) debian/$(PROG).postinst)
