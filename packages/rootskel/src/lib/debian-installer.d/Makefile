dir = lib/debian-installer.d

DEB_HOST_GNU_CPU    ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE   ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

files += \
	S60frontend

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
  ifeq ($(DEB_HOST_GNU_CPU),s390)
    files += \
	S30term-linux-s390
  else
    files += \
	S30term-linux \
	S35framebuffer-linux
  endif
  ifeq ($(DEB_HOST_GNU_CPU),i386)
    files += \
	S33framebuffer-module-linux-i386
  endif

  files += \
	S40utf8-linux \
	S70menu-linux \
	S72menu-exit \
	S90utf8-linux
endif

include ../../../Makefile.inc
