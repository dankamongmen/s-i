dir = etc/rcS.d

DEB_HOST_ARCH       ?= $(shell dpkg-architecture -qDEB_HOST_ARCH)

files = \
	S05init-pid \
	S50sysklog

ifeq ($(DEB_HOST_ARCH),i386)
# workaround
files += \
	S10unix
endif

include ../../Makefile.inc
