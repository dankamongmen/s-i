# This is just the bootstrapping makefile; the one to build udpkg is makefile

all: configure
	./configure && make

configure: configure.in
	autoconf

.PHONY: clean distclean
