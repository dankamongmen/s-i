dir =

subdirs = \
	etc \
	lib \
	sbin \
	var

subdirs_empty = \
	dev \
	media \
	initrd \
	mnt \
	proc \
	sys

ifeq ($(DEB_HOST_ARCH_OS),linux)
  files_exec = \
	init
endif

include ../Makefile.inc
