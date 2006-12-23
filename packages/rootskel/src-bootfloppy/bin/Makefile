ARCH := $(shell dpkg --print-architecture)

dir = bin
files_exec = \
	sh:/usr/lib/klibc/bin/sh.shared \
	mount:/usr/lib/klibc/bin/mount \
	insmod:/usr/lib/klibc/bin/insmod \
	mknod:/usr/lib/klibc/bin/mknod \
	mkdir:/usr/lib/klibc/bin/mkdir \
	cat:/usr/lib/klibc/bin/cat \
	umount:/usr/lib/klibc/bin/umount \
	zcat:/usr/lib/klibc/bin/zcat \
	run-init:/usr/lib/klibc/bin/run-init \
	uname:/usr/lib/klibc/bin/uname \
	cpio \
	timeout_read

CC=klcc
CFLAGS= 
LDFLAGS=-shared -s
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=cpio timeout_read
LIBS=

ifeq ($(ARCH),i386)
CFLAGS += -DKLIBC_SELECT 
endif

build-local:: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

clean:
	-rm -f $(BIN) $(OBJS) *~

include ../../Makefile.inc
