INSTALL = /usr/bin/install -c
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

all:

install:
	$(INSTALL) -d $(DESTDIR)/usr/lib/prebaseconfig.d
	$(INSTALL_SCRIPT) prebaseconfig \
		$(DESTDIR)/usr/lib/prebaseconfig.d/15cdrom-detect

clean:
