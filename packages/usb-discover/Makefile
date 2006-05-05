ifneq (,$(wildcard /usr/share/discover/pci.lst))
DISCDB=/usr/share/discover/pci.lst
else
DISCDB=/lib/discover/pci.lst
endif

install:
	(cat ohci-pci.lst ; \
	 awk '$$3 ~ /usb-ohci/ { print $$1}' < $(DISCDB)) \
	| sort -u > $(DESTDIR)/ohci-pci.lst

	(cat uhci-pci.lst ; \
	 awk '$$3 ~ /usb-uhci/ { print $$1}' < $(DISCDB)) \
	| sort -u > $(DESTDIR)/uhci-pci.lst

	(cat ehci-pci.lst ; \
	 awk '$$3 ~ /ehci[-_]hcd/ { print $$1}' < $(DISCDB)) \
	| sort -u > $(DESTDIR)/ehci-pci.lst
