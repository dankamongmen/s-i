dir = etc

DEB_HOST_ARCH_CPU   := $(shell dpkg-architecture -qDEB_HOST_ARCH_CPU 2>/dev/null)
DEB_HOST_ARCH_OS    := $(shell dpkg-architecture -qDEB_HOST_ARCH_OS 2>/dev/null)

# Take account of old dpkg-architecture output.
ifeq ($(DEB_HOST_ARCH_CPU),)
  DEB_HOST_ARCH_CPU   := $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
  ifeq ($(DEB_HOST_ARCH_CPU),x86_64)
    DEB_HOST_ARCH_CPU   := amd64
  endif
endif
ifeq ($(DEB_HOST_ARCH_OS),)
  DEB_HOST_ARCH_OS    := $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
endif

subdirs = \
	gtk-2.0

ifneq ($(DEB_HOST_ARCH_CPU),powerpc)
   files = \
	directfbrc
else
   files = \
	directfbrc:directfbrc-powerpc
endif

include ../../Makefile.inc
