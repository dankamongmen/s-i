dir = lib/debian-installer

DEB_HOST_GNU_CPU    ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE   ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

files_exec = \
	exit \
	exit-command \
	menu

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
  ifeq ($(DEB_HOST_GNU_CPU),s390)
    files += \
	detect-console:detect-console-linux-s390
  else
    files += \
	detect-console:detect-console-linux
  endif
endif

include ../../../Makefile.inc
