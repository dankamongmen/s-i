CC = gcc
CFLAGS = -Os -fomit-frame-pointer
LDFLAGS = -lcrypt

all: gen-crypt

install:
	install gen-crypt network-console network-console-menu $(DESTDIR)/bin

clean:
	rm -f gen-crypt
