CC = gcc
CFLAGS = -Os -fomit-frame-pointer
LDFLAGS = -lcrypt

all: gen-crypt

install-network-console:
	install -d $(DESTDIR)/bin
	install -m755 gen-crypt network-console network-console-menu $(DESTDIR)/bin
	install -d $(DESTDIR)/etc/ssh
	install -m644 sshd_config $(DESTDIR)/etc/ssh
	install -d $(DESTDIR)/usr/lib/prebaseconfig.d/
	install -m755 prebaseconfig $(DESTDIR)/usr/lib/prebaseconfig.d/80network-console

clean:
	rm -f gen-crypt
