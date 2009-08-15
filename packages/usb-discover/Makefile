ifneq (,$(wildcard /usr/share/discover/pci.lst))
DISCDB=/usr/share/discover/pci.lst
else
DISCDB=/lib/discover/pci.lst
endif

pkglibdir := /usr/lib/usb-discover

all:

install:
	install -d $(DESTDIR)$(pkglibdir)

	(cat ohci-pci.lst ; \
	 awk '$$3 ~ /usb-ohci/ { print $$1}' < $(DISCDB)) \
	| sort -u > $(DESTDIR)$(pkglibdir)/ohci-pci.lst

	(cat uhci-pci.lst ; \
	 awk '$$3 ~ /usb-uhci/ { print $$1}' < $(DISCDB)) \
	| sort -u > $(DESTDIR)$(pkglibdir)/uhci-pci.lst

	(cat ehci-pci.lst ; \
	 awk '$$3 ~ /ehci[-_]hcd/ { print $$1}' < $(DISCDB)) \
	| sort -u > $(DESTDIR)$(pkglibdir)/ehci-pci.lst
