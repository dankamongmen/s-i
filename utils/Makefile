ifndef TARGETS
TARGETS=shell
endif

CFLAGS=-Wall  -Os -fomit-frame-pointer
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
