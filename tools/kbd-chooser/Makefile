
ifdef DEBUG
DEB_BUILD_OPTIONS := debug:${DEB_BUILD_OPTIONS}
endif

CFLAGS := -Wall  -I.
ARCH := $(shell dpkg --print-architecture)

OBJS := loadkeys.o findfile.o ksyms.o  getfd.o

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g  -DDEBUG=1
OBJS += xmalloc.o
STRIP= /bin/true
else
CFLAGS +=  -fomit-frame-pointer -Os
STRIPTOOL=strip
STRIP= $(STRIPTOOL) --remove-section=.note --remove-section=.comment
endif

LDOPTS=  -ldebconfclient -ldebian-installer

ifeq ($(ARCH),alpha)
CFLAGS += -DAT_KBD  -DUSB_KBD
OBJS   += at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),i386)
CFLAGS += -DAT_KBD  -DUSB_KBD 
OBJS   += at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),ia64)
CFLAGS += -DAT_KBD  -DUSB_KBD 
OBJS   += at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),mips)
CFLAGS += -DAT_KBD -DUSB_KBD
OBJS += at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),mipsel)
CFLAGS += -DAT_KBD -DUSB_KBD
OBJS += at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),sparc)
CFLAGS += -DSPARC -DUSB_KBD 
OBJS   += sparc-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),powerpc)
CFLAGS += -DPOWERPC -DUSB_KBD -DAT_KBD -DAMIGA_KBD 
OBJS += mac-kbd.o amiga-kbd.o at-kbd.o usb-kbd.o 
endif
ifeq ($(ARCH),arm)
CFLAGS += -DARM -DAT_KBD -DUSB_KBD 
OBJS += acorn-kbd.o at-kbd.o usb-kbd.o
endif
ifeq ($(ARCH),m68k)
CFLAGS += -DAMIGA_KBD -DATARI_KBD -DMAC_KBD -DAT_KBD
OBJS += amiga-kbd.o atari-kbd.o mac-kbd.o at-kbd.o
endif
ifeq ($(ARCH),hppa)
CFLAGS += -DAT_KBD -DUSB_KBD
OBJS += at-kbd.o usb-kbd.o
endif

all: kbd-chooser


kbd-chooser: ${OBJS} kbd-chooser.c
	$(CC) $(CFLAGS) kbd-chooser.c -o $@ $(OBJS) $(LDOPTS)
	$(STRIP) $@



# loadkeys.o: separate rule since the flex output does not permit -Wall
loadkeys.o:     loadkeys.c analyze.c
	$(CC) -c $(CFLAGS) $(DEFS) $<

clean:
	rm -f *~ *.o kbd-chooser analyze.c loadkeys.c demo demo.templates *#

install:
	mkdir -p ${DESTDIR}/usr/bin
	cp kbd-chooser ${DESTDIR}/usr/bin/kbd-chooser

.PHONY: demo
demo.templates: debian/templates
	sed -e 's/_Description:/Description:/' -e '/^#/d' < debian/templates > demo.templates

demo: demo.templates
	rm -f demo
	ln -s kbd-chooser demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo
	rm -f demo.templates demo


