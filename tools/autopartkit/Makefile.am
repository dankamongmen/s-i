SUBDIRS = autopartkit.d

bin_PROGRAMS = autopartkit

noinst_PROGRAMS = dumptable distribute_test

DUMMYCDEBCONF = \
	dummydebconfclient.c \
	dummydebconfclient.h
#CFLAGS = -DTEST

autopartkit_SOURCES = \
	autopartkit.h \
	distribute.c \
	loadpartitions.c \
	lvm.c \
	mapdevfs.c \
	parted-compat.h \
	autopartkit.c

dumptable_SOURCES = \
	loadpartitions.c \
	dumptable.c

distribute_test_SOURCES = \
	distribute.c \
	loadpartitions.c \
	distribute-test.c

tabledir = $(sysconfdir)/autopartkit
table_DATA = \
	default.table

EXTRA_DIST = $(table_DATA)

testdevice:
	dd if=/dev/zero of=$@ bs=1M count=2048

check: autopartkit-test testdevice
	echo -n > autopartkit.log
	$(GDB) ./autopartkit-test
	/sbin/fdisk -l testdevice
	@echo
	@echo "******************** fstab ********************"
	@echo
	@cat fstab
