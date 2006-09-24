dir = lib/debian-installer.d

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

files += \
	S30term \
	S60frontend \
	S65theme \
	S72menu-exit

ifeq ($(DEB_HOST_ARCH_CPU),sparc)
  files += \
	S34framebuffer-linux-sparc
endif

ifeq ($(DEB_HOST_ARCH_OS),linux)
  ifneq ($(DEB_HOST_ARCH_CPU),s390)
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
