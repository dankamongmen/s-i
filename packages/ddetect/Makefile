DEB_HOST_ARCH ?= $(shell dpkg-architecture -qDEB_HOST_ARCH)
DEB_HOST_GNU_CPU ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

CC = gcc
CFLAGS = -Wall -O2 -g
CFLAGS_ARCH = -DARCH=$(DEB_HOST_ARCH) -DCPU=$(DEB_HOST_GNU_CPU) -DCPU_TEXT='"$(DEB_HOST_ARCH)"' -DSYSTEM=$(DEB_HOST_GNU_SYSTEM)

bindir=$(DESTDIR)/bin/
didir=$(DESTDIR)/lib/debian-installer.d
pbcdir=$(DESTDIR)/usr/lib/prebaseconfig.d

INSTALL=install
INSTALL_DATA = ${INSTALL} -m 644

all: archdetect archdetect-deb

clean:
	rm -f *~
	rm -f *.o
	rm -f archdetect
	rm -f archdetect-deb

install: install-hw-detect install-ethdetect install-archdetect install-archdetect-deb

install-hw-detect: hw-detect.sh
	$(INSTALL) -d $(bindir)
	$(INSTALL) hw-detect.sh $(bindir)/hw-detect
	$(INSTALL) hotplug-pcmcia.sh $(bindir)/hotplug-pcmcia
	$(INSTALL) -d $(pbcdir)
	$(INSTALL) hw-detect.prebaseconfig $(pbcdir)/30hw-detect
ifeq ($(DEB_HOST_ARCH),powerpc)
	$(INSTALL) discover-mac-io.sh $(bindir)/discover-mac-io
endif

install-hw-detect-full:

install-ethdetect: ethdetect.sh
	$(INSTALL) -d $(bindir)
	$(INSTALL) ethdetect.sh $(bindir)/ethdetect

install-archdetect: archdetect
	$(INSTALL) -d $(bindir)
	$(INSTALL) archdetect $(bindir)

install-archdetect-deb: archdetect-deb
	$(INSTALL) -d $(bindir)
	$(INSTALL) archdetect-deb $(bindir)

ARCHDETECT_TYPE_FILE = archdetect-$(DEB_HOST_GNU_TYPE)

ifeq ($(wildcard $(ARCHDETECT_TYPE_FILE).c),)
ARCHDETECT_TYPE_FILE = archdetect-generic
endif

archdetect: $(ARCHDETECT_TYPE_FILE).o archdetect.o 
	${CC} ${LDFLAGS} -o $@ $^ -ldebconfclient

archdetect-deb: $(ARCHDETECT_TYPE_FILE).o archdetect-deb.o 
	${CC} ${LDFLAGS} -o $@ $^

archdetect.o archdetect-deb.o: %.o:%.c
	$(CC) $(CFLAGS) $(CFLAGS_ARCH) -c $<

