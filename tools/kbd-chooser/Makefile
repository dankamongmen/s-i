DESTDIR=

ifdef DEBUG
DEB_BUILD_OPTIONS := debug:${DEB_BUILD_OPTIONS}
endif

CFLAGS := -Wall -Os -I.
ARCH := $(shell dpkg --print-architecture)

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g  -DDEBUG=1
STRIP= /bin/true
else
CFLAGS +=  -fomit-frame-pointer
STRIPTOOL=strip
STRIP= $(STRIPTOOL) --remove-section=.note --remove-section=.comment
endif

LDOPTS=  -ldebconf -ldebian-installer


OBJS = loadkeys.o findfile.o ksyms.o  getfd.o xmalloc.o

ifeq ($(ARCH),i386)
CFLAGS += -DAT_KBD -DPREFERRED_KEYBOARD="PS/2" -DUSB_KBD
OBJS   += at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),sparc)
CFLAGS += -DSPARC -DPREFERRED_KEYBOARD="sparc"
OBJS   += sparc-kbd.o
endif
ifeq ($(ARCH),powerpc)
CFLAGS += -DPOWERPC -DPREFERRED_KEYBOARD="mac"
OBJS += mac-kbd.o amiga-kbd.o
endif

all: kbd-chooser


kbd-chooser: ${OBJS} kbd-chooser.c
	$(CC) $(CFLAGS) kbd-chooser.c -o $@ $(OBJS) $(LDOPTS)
	$(STRIP) $@



# loadkeys.o: separate rule since the flex output does not permit -Wall
loadkeys.o:     loadkeys.c analyze.c
	$(CC) -c $(CFLAGS) $(DEFS) $<

clean:
	rm -f *~ *.o kbd-chooser analyze.c loadkeys.c *#

install:
	mkdir -p ${DESTDIR}/usr/bin
	cp kbd-chooser ${DESTDIR}/usr/bin/kbd-chooser
