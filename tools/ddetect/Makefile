bindir=$(DESTDIR)/bin/
didir=$(DESTDIR)/lib/debian-installer.d

INSTALL=install
INSTALL_SCRIPT = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

all:

clean:
	$(RM) *~

install: install-hw-detect install-ethdetect 

install-hw-detect: hw-detect.sh
	$(INSTALL) -d $(bindir) $(didir)
	$(INSTALL_SCRIPT) hw-detect.sh $(bindir)/hw-detect

install-hw-detect-full:

install-ethdetect: ethdetect.sh
	$(INSTALL) -d $(DESTDIR)/bin
	$(INSTALL_SCRIPT) ethdetect.sh $(bindir)/ethdetect

	$(INSTALL) -d $(DESTDIR)/usr/lib/prebaseconfig.d
	$(INSTALL_SCRIPT) prebaseconfig \
		$(DESTDIR)/usr/lib/prebaseconfig.d/40ethdetect
