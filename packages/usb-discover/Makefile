dir = usr/lib/hw-detect

ifeq ($(DEB_HOST_GNU_SYSTEM),linux)
	files = \
		ohci-pci.lst \
		uhci-pci.lst
endif

all:

lists: $(files)

ohci-pci.lst: /usr/share/discover/pci.lst
	 awk '$$3 ~ /usb-ohci/ { print $$1}' < /usr/share/discover/pci.lst | \
	 	sort > $@.new && mv $@.new $@

uhci-pci.lst: /usr/share/discover/pci.lst
	 awk '$$3 ~ /usb-uhci/ { print $$1}' < /usr/share/discover/pci.lst | \
	 	sort > $@.new && mv $@.new $@
