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
	cpio

CC=klcc
CFLAGS= 
LDFLAGS=-shared -s
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=cpio
LIBS=


build-local:: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS) $(LIBS)

clean:
	-rm -f $(BIN) $(OBJS) *~

include ../../Makefile.inc
