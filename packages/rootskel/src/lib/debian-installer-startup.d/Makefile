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
  ifneq (,$(filter i386 x86_64,$(DEB_HOST_GNU_CPU)))
    files += \
	S05acpi-linux-x86 \
	S40framebuffer-module-linux-x86
   endif

  ifeq ($(DEB_HOST_GNU_CPU),powerpc)
    files += \
	S45keyboard-linux-powerpc
  endif
  
  ifneq (,$(filter mips mipsel,$(DEB_HOST_GNU_CPU)))
    files += \
	S40framebuffer-module-linux-mips
   endif

  ifneq (,$(filter alpha,$(DEB_HOST_GNU_CPU)))
    files += \
	S40framebuffer-module-linux-alpha
   endif
endif

include ../../../Makefile.inc
