PROGS=ethdetect

INSTALL_DIR=$(DESTDIR)/usr/bin/
CFLAGS=-Wall -Wstrict-prototypes -Os -fomit-frame-pointer 
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment 
LDFLAGS= -ldiscover -ldebconf -ldebian-installer


all: $(OBJS) $(PROGS)

ethdetect: ethdetect.c ethpci.lst
	$(CC) $< -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)
	$(STRIP) $@

clean:
	$(RM) *.o $(PROGS) *.lst

install:
	$(foreach PROG, $(PROGS), \
	cp $(PROG) debian/$(PROG).postinst)
	
	mkdir -p $(DESTDIR)/usr/share/ddetect/
	cp *.lst $(DESTDIR)/usr/share/ddetect/
	  

ethpci.lst:
	./lstgen -p > ethpci.lst

