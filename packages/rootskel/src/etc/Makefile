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

install-local:: $(outdir)
	ln -fs /proc/mounts $(outdir)/mtab

include ../../Makefile.inc
