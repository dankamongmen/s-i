SUBDIRS = autopartkit.d

bin_PROGRAMS = autopartkit

noinst_PROGRAMS = autopartkit_test dumptable distribute_test lvm_test

if TEST
DUMMYCDEBCONF = \
	dummydebconfclient.c \
	dummydebconfclient.h
endif

autopartkit_SOURCES = \
	$(DUMMYCDEBCONF) \
	autopartkit.h \
	distribute.c \
	loadpartitions.c \
	lvm.c \
	mapdevfs.c \
	makepath.c \
	parted-compat.h \
	autopartkit.c \
	evaluator.c

autopartkit_test_SOURCES = \
	$(autopartkit_SOURCES)

lvm_test_SOURCES = \
	lvm.c \
	dummylog.c \
	lvm_test.c

dumptable_SOURCES = \
	dummylog.c \
	loadpartitions.c \
	dumptable.c \
	evaluator.c

distribute_test_SOURCES = \
	dummylog.c \
	distribute.c \
	loadpartitions.c \
	distribute-test.c \
	evaluator.c

tabledir = $(sysconfdir)/autopartkit
table_DATA = \
	default.table \
	test.table

EXTRA_DIST = $(table_DATA)

testdevice:
	dd if=/dev/zero of=$@ bs=1M count=2048

check-autopartkit: autopartkit_test testdevice
	echo -n > autopartkit.log
	$(GDB) ./autopartkit_test
	/sbin/fdisk -l testdevice
	@echo
	@echo "******************** fstab ********************"
	@echo
	@cat fstab

check: distribute_test lvm_test
	./distribute_test $(top_srcdir)/test.table
	./lvm_test
