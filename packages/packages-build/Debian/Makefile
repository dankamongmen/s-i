DESTDIR :=
perllibdir = $(shell perl -MConfig -e 'print $$Config{vendorlib}')/Debian
INSTALL = install -m644

all:

install: install-local
install-local: Installer.pm
	mkdir -p $(DESTDIR)$(perllibdir)
	@list='$^'; for p in $$list; do \
	  echo " $(INSTALL) $$d$$p $(DESTDIR)$(perllibdir)/$$f"; \
	  $(INSTALL) $$d$$p $(DESTDIR)$(perllibdir)/$$f; \
	done

clean:
