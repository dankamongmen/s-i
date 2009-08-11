VERSION := $(shell dpkg-parsechangelog | awk '/Version:/ { print $$2 }')

PERLLIBDIR := $(shell perl -MConfig -e 'print $$Config{vendorlib}')/Debian/Debhelper

POD2MAN := pod2man -c Debhelper -r "$(VERSION)"

build:
	$(POD2MAN) dh_di_numbers dh_di_numbers.1
	$(POD2MAN) dh_di_kernel_gencontrol dh_di_kernel_gencontrol.1
	$(POD2MAN) dh_di_kernel_install dh_di_kernel_install.1

clean:
	rm -f *.1

install:
	install -d $(DESTDIR)/usr/bin $(DESTDIR)$(PERLLIBDIR)/Sequence
	install dh_di_numbers $(DESTDIR)/usr/bin/
	install dh_di_kernel_gencontrol $(DESTDIR)/usr/bin/
	install dh_di_kernel_install $(DESTDIR)/usr/bin/
	install -m 0644 lib/Debian/Debhelper/Sequence/*.pm $(DESTDIR)$(PERLLIBDIR)/Sequence/
