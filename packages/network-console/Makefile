CC = gcc
CFLAGS = -Os -fomit-frame-pointer
LDFLAGS = -lcrypt

all: gen-crypt

install:
	install -d $(DESTDIR)/bin
	install gen-crypt network-console network-console-menu $(DESTDIR)/bin
	install -d $(DESTDIR)/etc/ssh
	install -m644 sshd_config{,_target} $(DESTDIR)/etc/ssh
	install -d $(DESTDIR)/usr/lib/prebaseconfig.d/
	install -m755 prebaseconfig $(DESTDIR)/usr/lib/prebaseconfig.d/80prepare-network-console

clean:
	rm -f gen-crypt
