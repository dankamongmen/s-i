ifndef TARGETS
TARGETS=niccfg-manual
endif

LDOPTS=-ldebconf  -ld-i

PREFIX=$(DESTDIR)/usr/
CFLAGS=-Wall  -Os -fomit-frame-pointer
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

niccfg-manual: niccfg-manual.c
	$(CC) $(CFLAGS) $@.c  -o $@ $(INCS) $(LDOPTS)

clean:
	rm -f niccfg-manual
