# This is just the bootstrapping makefile

all: cdebconf

globalmakeflags:
	./configure

cdebconf: globalmakeflags
	$(MAKE) -C src

configure: configure.in
	autoconf

install:
	$(MAKE) -C src $@

clean: 
	-rm -f *-stamp
	-$(MAKE) -C src $@

distclean: clean
	-rm -f config.status config.log config.cache globalmakeflags 
	-rm -f src/makefile src/config.h

.PHONY: cdebconf clean distclean
