
ifdef DEBUG
DEB_BUILD_OPTIONS := debug:$(DEB_BUILD_OPTIONS)
endif

CFLAGS := -Wall  -I.
ARCH := $(shell dpkg --print-architecture)

OBJS := loadkeys.o findfile.o ksyms.o  getfd.o

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
CFLAGS += -g  -DDEBUG=1
OBJS += xmalloc.o
STRIP= /bin/true
else
CFLAGS +=  -DNDEBUG=1 -fomit-frame-pointer -Os
STRIPTOOL=strip
STRIP= $(STRIPTOOL) --remove-section=.note --remove-section=.comment
endif

LDOPTS=  -ldebconfclient -ldebian-installer

ifeq ($(ARCH),alpha)
CFLAGS += -DAT_KBD  -DUSB_KBD
KEYBOARDS := at usb
endif
ifeq ($(ARCH),i386)
CFLAGS += -DAT_KBD  -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(ARCH),ia64)
CFLAGS += -DAT_KBD  -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(ARCH),mips)
CFLAGS += -DAT_KBD -DUSB_KBD
KEYBOARDS := at usb
endif
ifeq ($(ARCH),mipsel)
CFLAGS += -DAT_KBD -DUSB_KBD
KEYBOARDS := at usb
endif
ifeq ($(ARCH),sparc)
CFLAGS += -DSPARC -DUSB_KBD 
KEYBOARDS := sparc usb
endif
ifeq ($(ARCH),powerpc)
CFLAGS += -DPOWERPC -DUSB_KBD -DAT_KBD -DAMIGA_KBD 
KEYBOARDS := at usb amiga
endif
ifeq ($(ARCH),arm)
CFLAGS += -DARM -DAT_KBD -DUSB_KBD 
KEYBOARDS := at arm usb
endif
ifeq ($(ARCH),m68k)
CFLAGS += -DAMIGA_KBD -DATARI_KBD -DMAC_KBD -DAT_KBD
KEYBOARDS := amiga atari mac at
endif
ifeq ($(ARCH),hppa)
CFLAGS += -DAT_KBD -DUSB_KBD
KEYBOARDS := at usb
endif

TEMPLATES := debian/templates-in $(patsubst %, debian/keyboard-%.templates, $(KEYBOARDS))
OBJS += $(patsubst %, %-kbd.o, $(KEYBOARDS))

all: kbd-chooser debian/kbd-chooser.templates

kbd-chooser: ${OBJS} kbd-chooser.c
	$(CC) $(CFLAGS) kbd-chooser.c -o $@ $(OBJS) $(LDOPTS)
	$(STRIP) $@

debian/kbd-chooser.templates:  $(TEMPLATES)
	cat $(TEMPLATES) >  $@
	
# loadkeys.o: separate rule since the flex output does not permit -Wall
loadkeys.o:     loadkeys.c analyze.c
	$(CC) -c $(CFLAGS) $(DEFS) $<

clean:
	rm -f *~ *.o kbd-chooser analyze.c loadkeys.c demo demo.templates *#
	rm -f debian/kbd-chooser.templates

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp kbd-chooser $(DESTDIR)/usr/bin/kbd-chooser

.PHONY: demo
demo.templates: debian/kbd-chooser.templates
	sed -e 's/_Description:/Description:/' -e '/^#/d' < $< > $@

demo: demo.templates kbd-chooser
	rm -f demo
	ln -s kbd-chooser demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo
	rm -f demo.templates demo


