dir = lib/debian-installer-startup.d

DEB_HOST_GNU_CPU    ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE   ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

files = \
	S01mount \
	S10syslog \
	S20templates \
	S30read-environment \
	S35term

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
  ifeq ($(DEB_HOST_GNU_CPU),i386)
    files += \
	S40framebuffer-module-linux-i386
   endif

  ifeq ($(DEB_HOST_GNU_CPU),powerpc)
    files += \
	S45keyboard-linux-powerpc
  endif
endif

include ../../../Makefile.inc
