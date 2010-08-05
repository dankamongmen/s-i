pkglibdir := /usr/lib/usb-discover

all:

install:
	install -d $(DESTDIR)$(pkglibdir)

	sort -u ohci-pci.lst > $(DESTDIR)$(pkglibdir)/ohci-pci.lst
	sort -u uhci-pci.lst > $(DESTDIR)$(pkglibdir)/uhci-pci.lst
	sort -u ehci-pci.lst > $(DESTDIR)$(pkglibdir)/ehci-pci.lst
