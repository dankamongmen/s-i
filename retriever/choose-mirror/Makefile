CFLAGS=-Wall -g -D_GNU_SOURCE -DWITH_HTTP -DWITH_FTP
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=choose-mirror
LIBS=-ldebconfclient

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN) debian/choose-mirror.templates

Mirrors.masterlist:
	if wget -nv 'http://cvs.debian.org/*checkout*/webwml/english/mirror/Mirrors.masterlist?rev=HEAD&cvsroot=webwml&content-type=text/plain' -O - > $@.new; then \
           if test -s $@.new; then \
              mv $@.new $@; \
           fi; \
        fi

debian/choose-mirror.templates:  Mirrors.masterlist
	./makeheader.pl template
	cat debian/templates-in debian/templates-countries > debian/choose-mirror.templates

mirrors_http.h: Mirrors.masterlist
	./makeheader.pl http
	
mirrors_ftp.h: Mirrors.masterlist
	./makeheader.pl ftp

choose-mirror.o: mirrors_http.h mirrors_ftp.h

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS) $(LDFLAGS)

strip: $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)

# Size optimized and stripped binary target.
small: CFLAGS:=-Os $(CFLAGS) -DSMALL
small: clean strip debian/choose-mirror.templates
	ls -l $(BIN)

ftp: CFLAGS:=-Os -Wall -g -D_GNU_SOURCE -DWITH_FTP -DSMALL
ftp: clean strip
	ls -l $(BIN)

http: CFLAGS:=-Os -Wall -g -D_GNU_SOURCE -DWITH_HTTP -DSMALL
http: clean strip
	ls -l $(BIN)

clean:
	rm -f $(BIN) $(OBJS) *~ mirrors_*.h
	rm -f debian/templates-countries
	rm -f demo demo.templates

reallyclean:
	rm -f debian/choose-mirror.templates Mirrors.masterlist

.PHONY: demo
demo: choose-mirror demo.templates
	ln -sf choose-mirror demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo

demo.templates: debian/choose-mirror.templates
	sed -e 's/_Description:/Description:/' -e 's/__Choices:/Choices:/' \
	    -e 's/_Default:/Default:/' -e '/^#/d' < $< > $@
