ifndef TARGETS
TARGETS=utils
endif
OBJECTS = exec.o mapdevfs.o shell.o utils.o

CFLAGS=-Wall -W -Os -fomit-frame-pointer
#CFLAGS=-Wall -W -O1 -ggdb
INSTALL=install
STRIPTOOL=strip
STRIP = $(STRIPTOOL) --remove-section=.note --remove-section=.comment

all: $(TARGETS)

utils: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ -ldebian-installer -ldebconfclient

strip: $(TARGETS)
	$(STRIP) $^

clean:
	rm -f $(OBJECTS) $(TARGETS)
