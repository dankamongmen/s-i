# This is just the bootstrapping makefile

all: cdebconf man

globalmakeflags:
	./configure

cdebconf: globalmakeflags
	$(MAKE) -C src

man:
	$(MAKE) -C man

configure: configure.in
	autoconf

install:
	$(MAKE) -C src $@
	$(MAKE) -C man $@

clean: 
	-rm -f *-stamp
	-$(MAKE) -C src $@

distclean: clean
	-rm -f config.status config.log config.cache globalmakeflags 
	-rm -f src/Makefile src/config.h man/Makefile

.PHONY: cdebconf clean distclean
