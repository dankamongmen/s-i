DESTDIR :=
configdir = /usr/share/debian-installer/build/config
INSTALL = install -m644

SUBDIRS = arch

FILES = \
	build-glibc \
	build-uclibc

all: $(FILES)

install: install-recursive

install-local: $(FILES)
	mkdir -p $(DESTDIR)$(configdir)
	@list='$^'; for p in $$list; do \
	  echo " $(INSTALL) $$d$$p $(DESTDIR)$(configdir)/$$f"; \
	  $(INSTALL) $$d$$p $(DESTDIR)$(configdir)/$$f; \
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

