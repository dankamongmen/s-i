INSTALL = /usr/bin/install -c
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

all:

install:
	$(INSTALL) -d $(DESTDIR)/bin
	$(INSTALL_SCRIPT) hwdetect $(DESTDIR)/bin/hwdetect
	# Installed in debian/rules instead
	#$(INSTALL_DATA) hwdetect.templates $(DESTDIR)/bin/hwdetect.templates

clean:
