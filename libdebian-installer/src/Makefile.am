SUBDIRS = system

INCLUDES = \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libdebian-installer.la

libdebian_installer_la_SOURCES = \
	exec.c \
	hash.c \
	list.c \
	log.c \
	mem.c \
	packages.c \
	parser.c \
	parser_rfc822.c \
	release.c \
	slist.c \
	string.c \
	utils.c

libdebian_installer_la_LIBADD = \
	system/libsystem.la

libdebian_installer_la_LDFLAGS = \
	-version-info 4:0:0 \
	-Wl,--version-script=$(srcdir)/libdebian-installer.ver

libdebian_installer_la_DEPENDENCIES = \
	$(srcdir)/libdebian-installer.ver \
	system/libsystem.la

