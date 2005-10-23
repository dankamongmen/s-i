USE_HTTP=1
#USE_FTP=1

CFLAGS=-Wall -g -D_GNU_SOURCE
templates=debian/choose-mirror.templates-in

ifeq (1,${USE_HTTP})
CFLAGS:=$(CFLAGS) -DWITH_HTTP
templates:=$(templates) debian/choose-mirror.templates.http-in
endif
ifeq (1,${USE_FTP})
CFLAGS:=$(CFLAGS) -DWITH_FTP
templates:=$(templates) debian/choose-mirror.templates.ftp-in
endif
templates:=$(templates) debian/choose-mirror.templates.both-in

OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=choose-mirror
LIBS=-ldebconfclient -ldebian-installer

# Derivative distributions may want to change these.
MIRRORLISTURL=http://cvs.debian.org/*checkout*/webwml/english/mirror/Mirrors.masterlist?rev=HEAD&cvsroot=webwml&content-type=text/plain
MASTERLIST=Mirrors.masterlist

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
else
CFLAGS:=$(CFLAGS) -Os -fomit-frame-pointer
endif

all: $(BIN) debian/choose-mirror.templates

ifdef MIRRORLISTURL
# Freshen Mirrors.masterlist file, but allow failure.
$(MASTERLIST): force-try-update
	if [ "$$ONLINE" != n ]; then \
		if wget -nv '$(MIRRORLISTURL)' -O - > $@.new && \
		   test -s $@.new; then \
        		mv $@.new $@; \
		else \
			rm -f $@.new; \
		fi; \
	fi
force-try-update: ;
endif

debian/httplist-countries: $(MASTERLIST)
	./mirrorlist httplist $^

debian/ftplist-countries: $(MASTERLIST)
	./mirrorlist ftplist $^

debian/choose-mirror.templates: $(MASTERLIST) $(templates) debian/httplist-countries debian/ftplist-countries
	# Grab ISO codes from iso-codes package
	./get-iso-codes
	# Build the templates
	./mktemplates $(templates)

mirrors_http.h: $(MASTERLIST)
	./mirrorlist http $^

mirrors_ftp.h: $(MASTERLIST)
	./mirrorlist ftp $^

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
	rm -f debian/templates-countries debian/httplist-countries debian/ftplist-countries
	rm -f demo demo.templates
	rm -rf debian/iso-codes debian/pobuild

reallyclean: clean
	rm -f debian/choose-mirror.templates
ifdef MIRRORLISTURL
	rm -f Mirrors.masterlist
endif

.PHONY: demo
demo: choose-mirror demo.templates
	ln -sf choose-mirror demo
	DEBCONF_DEBUG=developer /usr/share/debconf/frontend ./demo

demo.templates: debian/choose-mirror.templates
	po2debconf $< > $@
