INSTALL = /usr/bin/install -c
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

all:

install:
	$(INSTALL) -d $(DESTDIR)/usr/lib/base-installer.d \
		      $(DESTDIR)/usr/lib/finish-install.d
	$(INSTALL_SCRIPT) base-installer \
		$(DESTDIR)/usr/lib/base-installer.d/40cdrom-detect
	$(INSTALL_SCRIPT) finish-install \
		$(DESTDIR)/usr/lib/finish-install.d/15cdrom-detect

clean:
