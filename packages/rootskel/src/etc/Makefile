dir = etc

subdirs = init.d rcS.d

files = \
	fstab \
	group \
	inittab \
	modules.conf \
	nsswitch.conf \
	passwd

install-local:: $(outdir)
	ln -s /proc/mounts $(outdir)/mtab

include ../../Makefile.inc
