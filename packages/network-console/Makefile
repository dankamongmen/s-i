CC = gcc
CFLAGS = -Os -fomit-frame-pointer
LDFLAGS = -lcrypt

all: gen-crypt

install:
	install gen-crypt network-console network-console-menu $(DESTDIR)/bin
	install -m644 sshd_config $(DESTDIR)/etc/ssh

clean:
	rm -f gen-crypt
