PROGS = report-hw package-versions

INSTALL     = install
INSTALL_DATA= install -m 644

bindir      = /bin
libdir      = /usr/share
pkglibdir   = $(libdir)/bugreporter-udeb

all:

install:
	mkdir -p $(DESTDIR)$(bindir)
	for p in $(PROGS) ; do \
	    $(INSTALL) $$p $(DESTDIR)$(bindir)/$$p ; \
	done
	$(INSTALL_DATA) install-report.template $(DESTDIR)$(pkglibdir)/install-report.template

dist:
	debuild -us -uc

clean:
	$(RM) *~
