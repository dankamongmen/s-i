
ifdef DEBUG
DEB_BUILD_OPTIONS := debug:$(DEB_BUILD_OPTIONS)
endif

CFLAGS := -Wall  -I.
DEB_HOST_ARCH := $(shell dpkg-architecture -qDEB_HOST_ARCH)

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

ifeq ($(DEB_HOST_ARCH),alpha)
CFLAGS += -DAT_KBD  -DUSB_KBD
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),i386)
CFLAGS += -DAT_KBD  -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),amd64)
CFLAGS += -DAT_KBD  -DUSB_KBD
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),ia64)
CFLAGS += -DAT_KBD  -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),mips)
CFLAGS += -DAT_KBD -DUSB_KBD
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),mipsel)
CFLAGS += -DAT_KBD -DUSB_KBD -DDEC_KBD
KEYBOARDS := at usb dec
endif
ifeq ($(DEB_HOST_ARCH),sparc)
CFLAGS += -DAT_KBD -DSPARC_KBD -DUSB_KBD 
KEYBOARDS := at usb sparc
endif
ifeq ($(DEB_HOST_ARCH),powerpc)
CFLAGS +=  -DUSB_KBD -DAT_KBD -DAMIGA_KBD -DMAC_KBD
KEYBOARDS := at usb amiga mac
endif
ifeq ($(DEB_HOST_ARCH),arm)
CFLAGS +=  -DAT_KBD -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),armel)
CFLAGS +=  -DAT_KBD -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),armeb)
CFLAGS +=  -DAT_KBD -DUSB_KBD 
KEYBOARDS := at usb
endif
ifeq ($(DEB_HOST_ARCH),m68k)
CFLAGS += -DAMIGA_KBD -DATARI_KBD -DMAC_KBD -DAT_KBD -DSPARC_KBD -D__m68k__
KEYBOARDS := amiga atari mac at sparc
endif
# For the moment, don't include HIL keyboards
ifeq ($(DEB_HOST_ARCH),hppa)
CFLAGS += -DAT_KBD -DUSB_KBD
KEYBOARDS := at usb
endif

TEMPLATES := debian/kbd-chooser.templates-in $(patsubst %, debian/keyboard-%.templates, $(KEYBOARDS))
OBJS += $(patsubst %, %-kbd.o, $(KEYBOARDS))

all: kbd-chooser kbd-mode debian/kbd-chooser.templates

kbd-chooser: ${OBJS} kbd-chooser.c
	$(CC) $(CFLAGS) kbd-chooser.c -o $@ $(OBJS) $(LDOPTS)
	$(STRIP) $@

kbd-mode: getfd.o kbd-mode.c
	$(CC) $(CFLAGS) kbd-mode.c -o $@ getfd.o $(LDOPTS)
	$(STRIP) $@

debian/kbd-chooser.templates:  $(TEMPLATES)
	cat $(TEMPLATES) >  $@
	# give the new templates file the same mtime as the input file, so
	# that po2debconf doesn't decide that it needs to run
	# debconf-updatepo
	touch -mr debian/kbd-chooser.templates-in $@
	
loadkeys.o:     loadkeys.c analyze.c

clean:
	rm -f *~ *.o kbd-mode kbd-chooser analyze.c loadkeys.c demo demo.templates *#
	rm -f debian/kbd-chooser.templates

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp kbd-chooser $(DESTDIR)/usr/bin/kbd-chooser
	cp kbd-mode $(DESTDIR)/usr/bin/kbd-mode

.PHONY: demo
demo.templates: debian/kbd-chooser.templates
	po2debconf $< > $@

demo: demo.templates kbd-chooser
	rm -f demo
	ln -s kbd-chooser demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo
	rm -f demo.templates demo


