DESTDIR :=
makedir = /usr/share/debian-installer/build/make
INSTALL = install -m644
INSTALL_EXEC = install

FILES = \
	config \
	rules

FILES_EXEC = \
	script

all: $(FILES) $(FILES_EXEC)

install: install-local install-local-exec

install-local: $(FILES)
	mkdir -p $(DESTDIR)$(makedir)
	@list='$^'; for p in $$list; do \
	  echo " $(INSTALL) $$d$$p $(DESTDIR)$(makedir)/$$f"; \
	  $(INSTALL) $$d$$p $(DESTDIR)$(makedir)/$$f; \
	done

install-local-exec: $(FILES_EXEC)
	mkdir -p $(DESTDIR)$(makedir)
	@list='$^'; for p in $$list; do \
	  echo " $(INSTALL_EXEC) $$d$$p $(DESTDIR)$(makedir)/$$f"; \
	  $(INSTALL_EXEC) $$d$$p $(DESTDIR)$(makedir)/$$f; \
	done

clean:
