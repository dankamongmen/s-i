ifndef TARGETS
TARGETS=shell mapdevfs
endif

CFLAGS=-Wall -D_GNU_SOURCE -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

shell: shell.c
	$(CC) $(CFLAGS) $@.c  -o $@  -ldebconf

strip: $(TARGETS)
	$(STRIP) $^

clean:
	rm -f $(TARGETS)
