DESTDIR :=
configdir = /usr/share/debian-installer/build/config/arch
INSTALL = install -m644

FILES = \
	linux

all: $(FILES)

install: install-local

install-local: $(FILES)
	mkdir -p $(DESTDIR)$(configdir)
	@list='$^'; for p in $$list; do \
	  echo " $(INSTALL) $$d$$p $(DESTDIR)$(configdir)/$$f"; \
	  $(INSTALL) $$d$$p $(DESTDIR)$(configdir)/$$f; \
	done

clean:
