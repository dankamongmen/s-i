DEB_HOST_ARCH ?= $(shell dpkg-architecture -qDEB_HOST_ARCH)
DEB_HOST_GNU_CPU ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

CC = gcc
CFLAGS = -Wall -O2 -g --std=c99 -DARCH=$(DEB_HOST_ARCH) -DCPU=$(DEB_HOST_GNU_CPU) -DCPU_TEXT='"$(DEB_HOST_GNU_CPU)"' -DSYSTEM=$(DEB_HOST_GNU_SYSTEM)
LDFLAGS = -ldebconfclient

bindir=$(DESTDIR)/bin/
didir=$(DESTDIR)/lib/debian-installer.d

INSTALL=install
INSTALL_DATA = ${INSTALL} -m 644

all: archdetect

clean:
	$(RM) -f *~
	rm -f *.o
	rm -f archdetect

install: install-hw-detect install-ethdetect install-archdetect

install-hw-detect: hw-detect.sh
	$(INSTALL) -d $(bindir)
	$(INSTALL) hw-detect.sh $(bindir)/hw-detect

install-hw-detect-full:

install-ethdetect: ethdetect.sh
	$(INSTALL) -d $(bindir)
	$(INSTALL) ethdetect.sh $(bindir)/ethdetect

	$(INSTALL) -d $(DESTDIR)/usr/lib/prebaseconfig.d
	$(INSTALL) prebaseconfig \
		$(DESTDIR)/usr/lib/prebaseconfig.d/40ethdetect

install-archdetect: archdetect
	$(INSTALL) -d $(bindir)
	$(INSTALL) archdetect $(bindir)

ARCHDETECT_TYPE_FILE = archdetect-$(DEB_HOST_GNU_TYPE).o

ifeq (,$(wildcard $(ARCHDETECT_TYPE_FILE)))
ARCHDETECT_TYPE_FILE = archdetect-generic.o
endif

archdetect: $(ARCHDETECT_TYPE_FILE) archdetect.o
	${CC} ${LDFLAGS} -o archdetect $^

%.o:%.c
	$(CC) $(CFLAGS) -c $<

