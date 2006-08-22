dir = etc

# backward compatibility
subdirs_empty = rcS.d

files = \
	fstab \
	group \
	inittab \
	modules.conf \
	nsswitch.conf \
	passwd \
	profile

install-local:: $(outdir)
	ln -fs /proc/mounts $(outdir)/mtab

include ../../Makefile.inc
