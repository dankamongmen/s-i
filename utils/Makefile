ifndef TARGETS
TARGETS=di-utils debian/di-utils-shell.postinst
endif
OBJECTS = mapdevfs.o utils.o

CFLAGS=-Wall -W -Os -fomit-frame-pointer
#CFLAGS=-Wall -W -O1 -ggdb
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

di-utils: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ -ldebian-installer

debian/di-utils-shell.postinst: debian/di-utils-shell.postinst.c
	$(CC) $(CFLAGS) $^ -o $@ -ldebconfclient

strip: $(TARGETS)
	$(STRIP) $^

clean:
	rm -f $(OBJECTS) $(TARGETS)
