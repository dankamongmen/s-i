dir = lib/debian-installer

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

files = \
	init-debug

files_exec = \
	exit:exit-$(DEB_HOST_ARCH_OS) \
	exit-command \
	menu

ifeq ($(DEB_HOST_ARCH),s390)
  files += \
	detect-console:detect-console-linux-s390
else
  files += \
	detect-console:detect-console-$(DEB_HOST_ARCH_OS)
endif

include ../../../Makefile.inc
