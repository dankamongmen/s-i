ifndef TARGETS
TARGETS=niccfg-manual
endif

INCS=-I../cdebconf/src/
LDOPTS=-L../cdebconf/src -ldebconf -Wl,-rpath,../cdebconf/src 

PREFIX=$(DESTDIR)/usr/
CFLAGS=-Wall  -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

niccfg-manual: niccfg-manual.c utils.o
	$(CC) $(CFLAGS) $@.c  -o $@ $(INCS) $(LDOPTS) utils.o

clean:
	rm -f niccfg-manual *.o 
