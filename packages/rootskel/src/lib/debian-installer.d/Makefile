dir = lib/debian-installer.d

DEB_HOST_GNU_CPU    ?= $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
DEB_HOST_GNU_SYSTEM ?= $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
DEB_HOST_GNU_TYPE   ?= $(shell dpkg-architecture -qDEB_HOST_GNU_TYPE)

files_menu =
files_term = 

extra_files =

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
  files_menu = \
	S70main-menu-linux

  files_term = \
	S30term-linux \
	S35framebuffer-linux \
	S40utf8-linux \
	S90utf8-linux

  ifeq ($(DEB_HOST_GNU_CPU),i386)
    files_term += \
	S33framebuffer-module-linux-i386
  endif
  ifeq ($(DEB_HOST_GNU_CPU),s390)
    files_term = \
	S30term-linux-s390
  endif
endif

files = \
	S20templates \
	S25env2debconf \
	S50frontend \
	S50priority

files += \
	$(files_menu) \
	$(files_term) \
	$(extra_files)

include ../../Makefile.inc
