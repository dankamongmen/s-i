dir = lib/debian-installer.d

DEB_HOST_GNU_CPU    ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE   ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

files += \
	S30term \
	S60frontend \
	S72menu-exit

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
  ifneq ($(DEB_HOST_GNU_CPU),s390)
    files += \
	S35framebuffer-linux
  endif

  files += \
	S40term-linux \
	S70menu-linux
else
  files += \
	S70menu
endif

include ../../../Makefile.inc
