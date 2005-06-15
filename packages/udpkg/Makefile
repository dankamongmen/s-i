# This is just the bootstrapping makefile; the one to build udpkg is makefile

all: configure
	./configure && make

configure: configure.ac
	autoconf

.PHONY: clean distclean
