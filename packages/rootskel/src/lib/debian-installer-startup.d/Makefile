dir = lib/debian-installer-startup.d

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
	S01mount \
	S02module-params \
	S02udev-fixup \
	S03hotplug \
	S10syslog \
	S20templates \
	S35term

ifeq ($(DEB_HOST_ARCH_OS),linux)
  ifneq (,$(filter i386 amd64,$(DEB_HOST_ARCH_CPU)))
    files += \
	S05acpi-linux-x86 \
	S40framebuffer-module-linux-x86 \
	S04countcpus-linux-x86
   endif
  
  ifeq ($(DEB_HOST_ARCH_CPU),powerpc)
    files += \
	S45keyboard-linux-powerpc
  endif
  
  ifneq (,$(filter mips mipsel,$(DEB_HOST_ARCH_CPU)))
    files += \
	S40framebuffer-module-linux-mips
   endif

  ifneq (,$(filter alpha,$(DEB_HOST_ARCH_CPU)))
    files += \
	S40framebuffer-module-linux-alpha \
	S04countcpus-linux-alpha
   endif
endif

include ../../../Makefile.inc
