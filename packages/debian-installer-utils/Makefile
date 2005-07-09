ifndef TARGETS
TARGETS=mapdevfs
endif

# Where to create device entries for di-utils-devicefiles
ifndef DEVDIR
DEVDIR=debian/di-utils-devicefiles/dev
endif

CFLAGS=-Wall -W -Os -fomit-frame-pointer -g
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

mapdevfs: mapdevfs.c
	$(CC) $(CFLAGS) $^ -o $@ -ldebian-installer

devicefiles:
	mkdir -p $(DEVDIR)/dev
	mknod $(DEVDIR)/dev/console c 5 1
	mkdir -p $(DEVDIR)/dev/vc
	mknod $(DEVDIR)/dev/vc/0 c 4 0
	mknod $(DEVDIR)/dev/vc/1 c 4 1
	mknod $(DEVDIR)/dev/vc/2 c 4 2
	mknod $(DEVDIR)/dev/vc/3 c 4 3
	mknod $(DEVDIR)/dev/vc/4 c 4 4
	mknod $(DEVDIR)/dev/vc/5 c 4 5
	mkdir -p $(DEVDIR)/dev/rd
	mknod $(DEVDIR)/dev/rd/0 b 1 0

strip: $(TARGETS)
	$(STRIP) $^

clean:
	rm -f $(OBJECTS) $(TARGETS)
