
# define this if you want to link against uC-libc
#UC_LIBC=yes


# uncomment this is you want this linked statically 
#LINK_STATICALLY=yes

ifdef LINK_STATICALLY
LDFLAGS=-Wl,--static
endif


OBJS=ddetect.o

ifndef PROGS
PROGS=memdetect diskdetect mdmdetect cddetect cpudetect ethdetect snddetect
endif
HEADERS= ethdetect.h snddetect.h cddetect.h mdmdetect.h

INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall  -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment 
LDFLAGS=-ldetect -lisapnp


all: lst2header $(HEADERS) $(OBJS) $(PROGS)
	$(foreach PROG, $(PROGS), \
	($(STRIP) $(PROG)) ; )	

mdmdetect: mdmdetect.c ddetect.o
cddetect : cddetect.c ddetect.o
ethdetect :ethdetect.c ddetect.o
snddetect: snddetect.c ddetect.o

lst2header: lst2header.c
	$(CC) lst2header.c -o lst2header -ldetect -lisapnp 

ethdetect.h: lst2header
	-./lst2header -e > $@ 
snddetect.h: lst2header
	-./lst2header -s > $@ 
cddetect.h: lst2header
	-./lst2header -c > $@ 
mdmdetect.h: lst2header
	-./lst2header -m > $@


clean:
	$(RM) *.o $(PROGS) $(HEADERS) lst2header

install:
	mkdir -p $(INSTALL_DIR)
	$(INSTALL) $(PROGS) $(INSTALL_DIR)


ifdef UC_LIBC

UC_INC=/usr/home/davidw/debian/uclib/uC-libc/include/

#CFLAGS=-g -Wall -nostdinc -I$(UC_INC) -I$(UC_INC)linux -I/usr/include
CFLAGS=-g -Wall

#LIBC=/usr/home/davidw/debian/uclib/uC-libc/libc.a

OBJS=ddetect.o ddetect_nopassive.o

%.o: %.c
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@

%:%.c %.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) -ldetect -lisapnp
	$(STRIP) $@	
	@chmod +x $@ 
	@size $@	


ddetect: $(PROG).c $(OBJS)
#	$(CC) -nostdlib -luC -ldetect ddetect.c -o ddetect	
	$(LD) $(OBJS) $(LIBC) --static -o $(PROG) -L. -ldetect  -luc
	$(STRIP) $(PROG)	
	@chmod +x  $(PROG)
	@size $(PROG)

test: test.o
	$(CC) -nostdlib -s -gc-sections test.o $(LIBC) -o test -lgcc

endif
