CC = gcc
CFLAGS = -Os -fomit-frame-pointer
LDFLAGS = -lcrypt

all: gen-crypt

install-network-console:
	install -d $(DESTDIR)/bin
	install -m755 gen-crypt network-console network-console-menu $(DESTDIR)/bin
	install -d $(DESTDIR)/etc/ssh
	install -m644 sshd_config{,_target} $(DESTDIR)/etc/ssh
	install -d $(DESTDIR)/usr/lib/prebaseconfig.d/
	install -m755 prebaseconfig $(DESTDIR)/usr/lib/prebaseconfig.d/80prepare-network-console

install-network-console-config:
	install -d $(DESTDIR)/usr/lib/base-config/menu
	install -m755 base-config $(DESTDIR)/usr/lib/base-config/menu/network-console
	install -m644 base-config.mnu $(DESTDIR)/usr/lib/base-config/menu/network-console.mnu
	install -d $(DESTDIR)/usr/sbin
	install -m755 base-config-network-console $(DESTDIR)/usr/sbin

clean:
	rm -f gen-crypt
