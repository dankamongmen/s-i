# TODO: the ../../tools/cdebconf bits are temporary, until there is a -dev
# package.
CFLAGS=-Wall -g -D_GNU_SOURCE -I../../tools/cdebconf/src
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=choose-mirror
LIBS=-L../../tools/cdebconf/src -ldebconf

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN)

# This is evil. There is no decent way to download this file.
Mirrors.masterlist:
	if cvs -d":pserver:anonymous@cvs.debian.org:/cvs/webwml" \
	   co -p webwml/english/mirror/Mirrors.masterlist > Mirrors.masterlist.0 \
	   && [ -s "Mirrors.masterlist.0" ]; then \
		mv -f Mirrors.masterlist.0 Mirrors.masterlist > /dev/null 2>&1; \
	fi

mirrors_http.h: Mirrors.masterlist
	./makeheader.pl http
	
mirrors_ftp.h: Mirrors.masterlist
	./makeheader.pl ftp

choose-mirror.c: mirrors_http.h

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)

# Size optimized and stripped binary target.
small: CFLAGS=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~ mirrors_*.h
