dir = lib/debian-installer.d

DEB_HOST_GNU_CPU    ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE   ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

extra_files =

support_bterm = false
support_framebuffer = false
support_utf8 = false

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
  support_utf8 = linux

  ifeq ($(DEB_HOST_GNU_CPU),i386)
    support_framebuffer = linux-i386
  endif
  ifeq ($(DEB_HOST_GNU_CPU),s390)
    extra_files = S30term-linux-s390

    support_utf8 = false
  endif
endif

files = \
	S20templates \
	S50frontend \
	S50priority

ifneq ($(support_framebuffer),false)
support_bterm = true
files += \
	S40framebuffer-$(support_framebuffer)
endif

ifneq ($(support_utf8),false)
files += \
	S30utf8-$(support_utf8) \
	S90utf8-$(support_utf8)
endif

ifneq ($(support_bterm),false)
files += \
	S42bterm \
	S70main-menu-bterm
else
files += \
	S70main-menu
endif

files += $(extra_files)

include ../../Makefile.inc
