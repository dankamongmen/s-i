SUBDIRS = system

INCLUDES = \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libdebian-installer.la

libdebian_installer_la_SOURCES = \
	exec.c \
	hash.c \
	log.c \
	mem.c \
	package.c \
	package_parser.c \
	packages.c \
	packages_parser.c \
	parser.c \
	parser_rfc822.c \
	slist.c \
	string.c \
	utils.c
#	release.c
#	list.c

libdebian_installer_la_LIBADD = \
	system/libsystem.la

libdebian_installer_la_LDFLAGS = \
	-version-info 4:0:0 \
	-Wl,--version-script=$(srcdir)/libdebian-installer.ver

libdebian_installer_la_DEPENDENCIES = \
	$(srcdir)/libdebian-installer.ver \
	system/libsystem.la

