DEB_HOST_ARCH_OS ?= $(shell dpkg-architecture -qDEB_HOST_ARCH_OS 2>/dev/null)

dir = etc

# backward compatibility
subdirs_empty = rcS.d

files = \
	fstab:fstab-$(DEB_HOST_ARCH_OS) \
	group \
	inittab:inittab-$(DEB_HOST_ARCH_OS) \
	modules.conf \
	nsswitch.conf \
	passwd \
	profile

# On Hurd, /proc/mounts won't exist or will be a symlink to /etc/mtab itself.
install-local:: $(outdir)
ifeq ($(DEB_HOST_ARCH_OS),hurd)
	ln -fs /proc/mounts $(outdir)/mtab
endif

include ../../Makefile.inc
