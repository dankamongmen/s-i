CFLAGS=-Wall -g -D_GNU_SOURCE -DWITH_HTTP -DWITH_FTP
OBJS=$(subst .c,.o,$(wildcard *.c))
BIN=choose-mirror
LIBS=-ldebconf

ifdef DEBUG
CFLAGS:=$(CFLAGS) -DDODEBUG
endif

all: $(BIN)

Mirrors.masterlist:
	if wget -nv 'http://cvs.debian.org/*checkout*/webwml/english/mirror/Mirrors.masterlist?rev=HEAD&cvsroot=webwml&content-type=text/plain' -O - > $@.new; then \
           if test -s $@.new; then \
              mv $@.new $@; \
           fi; \
        fi

mirrors_http.h: Mirrors.masterlist
	./makeheader.pl http
	
mirrors_ftp.h: Mirrors.masterlist
	./makeheader.pl ftp

choose-mirror.c: mirrors_http.h mirrors_ftp.h

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS) $(LDFLAGS)

# Size optimized and stripped binary target.
small: CFLAGS:=-Os $(CFLAGS) -DSMALL
small: clean $(BIN)
	strip --remove-section=.comment --remove-section=.note $(BIN)
	ls -l $(BIN)

clean:
	-rm -f $(BIN) $(OBJS) *~ mirrors_*.h
