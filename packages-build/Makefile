DESTDIR :=
bindir = /usr/bin
mandir = /usr/share/man/man1
INSTALL = install

SUBDIRS = \
	Debian \
	make

DEBONLY = \
	dh_di_installchangelogs \
	dh_di_installdocs \
	dh_di_makeshlibs \
	dh_di_md5sums

STANDARD = \
	dh_di_builddeb \
	dh_di_debonly \
	dh_di_gencontrol

all: $(DEBONLY) $(STANDARD:=.1) $(DEBONLY:=.1)

$(DEBONLY): dh_di_debonly
	ln -sf $< $@

$(DEBONLY:=.1): %.1: dh_di_debonly.1
	ln -sf $< $@

$(STANDARD:=.1): %.1: %
	pod2man -c Debhelper -r '' $< $@

install: install-recursive

install-local: install-local-bin install-local-man

install-local-bin: $(DEBONLY) $(STANDARD)
	mkdir -p $(DESTDIR)$(bindir)
	@list='$^'; for p in $$list; do \
	  if [ -h $$p ]; then \
	    echo " cp -a $$d$$p $(DESTDIR)$(bindir)/$$f"; \
	    cp -a $$d$$p $(DESTDIR)$(bindir)/$$f; \
	  else \
	    echo " $(INSTALL) $$p $(DESTDIR)$(bindir)/$$f"; \
	    $(INSTALL) $$p $(DESTDIR)$(bindir)/$$f; \
	  fi \
	done

install-local-man: $(DEBONLY:=.1) $(STANDARD:=.1)
	mkdir -p $(DESTDIR)$(mandir)
	@list='$^'; for p in $$list; do \
	  if [ -h $$p ]; then \
	    echo " cp -a $$d$$p $(DESTDIR)$(mandir)/$$f"; \
	    cp -a $$d$$p $(DESTDIR)$(mandir)/$$f; \
	  else \
	    echo " $(INSTALL) $$p $(DESTDIR)$(mandir)/$$f"; \
	    $(INSTALL) $$p $(DESTDIR)$(mandir)/$$f; \
	  fi \
	done

clean-recursive install-recursive:
	@dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-local"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-local" || exit 1; \
	fi; test -z "$$fail"

clean: clean-recursive

clean-local:
	-rm -f $(DEBONLY) $(DEBONLY:=.1) $(STANDARD:=.1)

