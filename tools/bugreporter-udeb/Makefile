PROGS = report-hw package-versions

INSTALL     = install
INSTALL_DATA= install -m 644

bindir      = /bin
libdir      = /usr/lib
pkglibdir   = $(libdir)/bugreporter-udeb

all:

install:
	mkdir -p $(DESTDIR)$(bindir)
	for p in $(PROGS) ; do \
	    $(INSTALL) $$p $(DESTDIR)$(bindir)/$$p ; \
	done

dist:
	debuild -us -uc

clean:
	$(RM) *~
