DISCDB=/usr/share/discover/pci.lst

install:
	(cat ohci-pci.lst ; \
	 awk '$$3 ~ /usb-ohci/ { print $$1}' < $(DISCDB)) \
	| sort > $(DESTDIR)/ohci-pci.lst

	(cat uhci-pci.lst ; \
	 awk '$$3 ~ /usb-uhci/ { print $$1}' < $(DISCDB)) \
	| sort > $(DESTDIR)/uhci-pci.lst

	(cat ehci-pci.lst ; \
	 awk '$$3 ~ /ehci[-_]hcd/ { print $$1}' < $(DISCDB)) \
	| sort > $(DESTDIR)/ehci-pci.lst
