ifndef TARGETS
TARGETS=shell mapdevfs
endif

CFLAGS=-Wall -D_GNU_SOURCE -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

mapdevfs: mapdevfs.c
	$(CC) $(CFLAGS) $^ -o $@ -ldebian-installer

shell: shell.c
	$(CC) $(CFLAGS) $^ -o $@ -ldebconfclient

strip: $(TARGETS)
	$(STRIP) $^

clean:
	rm -f $(TARGETS)
