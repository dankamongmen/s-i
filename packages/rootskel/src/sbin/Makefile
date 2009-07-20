
DEB_HOST_ARCH_OS ?= $(shell dpkg-architecture -qDEB_HOST_ARCH_OS 2>/dev/null)

dir = sbin

files_exec = \
	debian-installer \
	debian-installer-startup \
	shutdown \
	init:init-$(DEB_HOST_ARCH_OS) \
	reopen-console:reopen-console-$(DEB_HOST_ARCH_OS) \
	steal-ctty

ifeq ($(DEB_HOST_ARCH_OS),linux)
  files_exec += \
	console-type
endif

console-type: console-type.c
	gcc -Os -Wall console-type.c -o console-type

steal-ctty: steal-ctty.c
	gcc -Os -Wall steal-ctty.c -o steal-ctty

ifeq ($(DEB_HOST_ARCH_OS),linux)
build: console-type steal-ctty
else
build: steal-ctty
endif

clean:
	rm -f console-type steal-ctty

include ../../Makefile.inc
