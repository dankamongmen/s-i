INSTALL     = install
INSTALL_DATA= install -m 644

libdir      = /usr/lib
pkglibdir   = $(libdir)/bugreporter-udeb

all:

install:
	mkdir -p $(DESTDIR)$(pkglibdir)
	$(INSTALL) report-hw $(DESTDIR)$(pkglibdir)/report-hw

dist:
	debuild -us -uc

clean:
	$(RM) *~
