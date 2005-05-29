DEB_HOST_ARCH ?= $(shell dpkg-architecture -qDEB_HOST_ARCH)
DEB_HOST_ARCH_CPU ?= $(shell dpkg-architecture -qDEB_HOST_ARCH_CPU 2>/dev/null || dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_ARCH_OS ?= $(shell dpkg-architecture -qDEB_HOST_ARCH_OS 2>/dev/null || dpkg-architecture -qDEB_HOST_GNU_SYSTEM)

CC = gcc
CFLAGS = -Wall -O2 -g
CFLAGS_ARCH = -DARCH=$(DEB_HOST_ARCH) -DCPU=$(DEB_HOST_ARCH_CPU) -DCPU_TEXT='"$(DEB_HOST_ARCH)"' -DSYSTEM=$(DEB_HOST_ARCH_OS)

bindir=$(DESTDIR)/bin/
hpdir=$(DESTDIR)/etc/hotplug.d
didir=$(DESTDIR)/lib/debian-installer.d
pbcdir=$(DESTDIR)/usr/lib/prebaseconfig.d

INSTALL=install
INSTALL_DATA = ${INSTALL} -m 644

all: archdetect

clean:
	rm -f *~
	rm -f *.o
	rm -f archdetect

install: install-hw-detect install-ethdetect install-archdetect install-archdetect-deb

install-hw-detect: hw-detect.sh
	$(INSTALL) -d $(bindir)
ifeq ($(DEB_HOST_ARCH_OS),linux)
	$(INSTALL) hw-detect.sh $(bindir)/hw-detect
	$(INSTALL) hotplug-pcmcia.sh $(bindir)/hotplug-pcmcia
	$(INSTALL) sysfs-update-devnames.sh $(bindir)/sysfs-update-devnames
	$(INSTALL) -d $(hpdir)/net
	$(INSTALL) net-hotplug.sh $(hpdir)/net/ddetect.hotplug
	$(INSTALL) -d $(pbcdir)
	$(INSTALL) hw-detect.prebaseconfig $(pbcdir)/30hw-detect
ifeq ($(DEB_HOST_ARCH),powerpc)
	$(INSTALL) discover-mac-io.sh $(bindir)/discover-mac-io
	$(INSTALL) discover-rs6k.sh $(bindir)/discover-rs6k
endif
else
	$(INSTALL) detect-stub.sh $(bindir)/hw-detect
endif

install-hw-detect-full:

install-ethdetect: ethdetect.sh
	$(INSTALL) -d $(bindir)
	$(INSTALL) ethdetect.sh $(bindir)/ethdetect

install-archdetect: archdetect
	$(INSTALL) -d $(bindir)
	$(INSTALL) archdetect $(bindir)

install-archdetect-deb: archdetect
	$(INSTALL) -d $(bindir)
	$(INSTALL) archdetect $(bindir)

ARCHDETECT_TYPE_FILE = archdetect-$(DEB_HOST_ARCH_CPU)-$(DEB_HOST_ARCH_OS)

ifeq ($(wildcard $(ARCHDETECT_TYPE_FILE).c),)
ARCHDETECT_TYPE_FILE = archdetect-generic
endif

archdetect: $(ARCHDETECT_TYPE_FILE).o archdetect.o 
	${CC} ${LDFLAGS} -o $@ $^

archdetect.o: %.o:%.c
	$(CC) $(CFLAGS) $(CFLAGS_ARCH) -c $<

