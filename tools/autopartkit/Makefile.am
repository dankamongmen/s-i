bin_PROGRAMS = autopartkit

noinst_PROGRAMS = dumptable distribute-test

autopartkit_SOURCES = \
	autopartkit.h \
	choosetable.c \
	distribute.c \
	loadpartitions.c \
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
	Server+LTSP-server.table \
	Server+Workstation.table \
	Server.table \
	Workstation+LTSP-server.table \
	Workstation.table \
	default.table

EXTRA_DIST = $(table_DATA)

foo = \
	dummydebconfclient.c \
	dummydebconfclient.h

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
