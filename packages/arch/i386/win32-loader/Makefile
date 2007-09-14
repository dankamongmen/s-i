
export SHELL := bash

PACKAGE	:= win32-loader
VERSION	:= $(shell head -n 1 debian/changelog | sed -e "s/^$(PACKAGE) (\(.*\)).*/\1/g")

CC		:= i586-mingw32msvc-gcc -Os
STRIP		:= i586-mingw32msvc-strip
MAKENSIS	:= makensis
ifdef NETWORK_BASE_URL
MAKENSIS	+= -DNETWORK_BASE_URL=$(NETWORK_BASE_URL)
endif

# partmap and fs
GRUB_MODULES	+= pc fat ntfs
# used in our grub.cfg
GRUB_MODULES	+= search linux _linux boot
# might be useful for debugging
GRUB_MODULES	+= cat cpuid _chain chain halt help ls reboot search

all: win32-loader.exe g2ldr g2ldr.mbr

core.img:
	grub-mkimage -o $@ --prefix / $(GRUB_MODULES)

g2ldr: /usr/lib/grubutil/g2hdr.bin core.img
	cat $^ > $@

g2ldr.mbr: /usr/lib/grubutil/g2ldr.mbr
	cp $^ $@

win32-loader.exe: main.nsi maps.ini \
		templates/binary_choice.ini templates/graphics.ini templates/custom.ini templates/4_choices.ini \
		plugins/cpuid/test64.dll plugins/systeminfo/systeminfo.dll plugins/string.dll \
		swirl.ico license
	$(MAKE) -C l10n
	$(MAKENSIS) main.nsi
	du -h win32-loader.exe

license: license.in
	sed -e "s/@VERSION@/$(VERSION)/g" < $^ > $@

plugins/cpuid/test64.dll: plugins/cpuid/cpuid.c plugins/cpuid/plugin.c
	$(CC) -Wl,--file-alignment,512 $^ -shared -o $@
	$(STRIP) $@

plugins/systeminfo/systeminfo.dll: plugins/systeminfo/systeminfo.c
	$(CC) -Wl,--file-alignment,512 $^ -shared -o $@
	$(STRIP) $@

plugins/string.dll: plugins/string.c
	$(CC) -Wl,--file-alignment,512 $^ -shared -o $@
	$(STRIP) $@

iso: win32-loader.iso
win32-loader.iso: win32-loader.exe win32-loader.ini autorun.inf \
  netboot/install.386/vmlinuz netboot/install.386/initrd.gz netboot/install.386/gtk/initrd.gz \
  netboot/install.amd/vmlinuz netboot/install.amd/initrd.gz netboot/install.amd/gtk/initrd.gz \
  g2ldr g2ldr.mbr
	cp g2ldr g2ldr.mbr netboot/
	todos < autorun.inf > netboot/autorun.inf
	todos < win32-loader.ini > netboot/win32-loader.ini
	cp win32-loader.exe netboot/
	# this mimics the location of html docs in official CDs
	echo "<html>hello, borg world!</html>" > netboot/README.html
	genisoimage -r -J -o $@ netboot

netboot/install.386/vmlinuz:
	mkdir -p netboot/install.386
	wget http://ftp.nl.debian.org/debian/dists/etch/main/installer-i386/current/images/netboot/debian-installer/i386/linux -O $@
netboot/install.386/initrd.gz:
	mkdir -p netboot/install.386
	wget http://ftp.nl.debian.org/debian/dists/etch/main/installer-i386/current/images/netboot/debian-installer/i386/initrd.gz -O $@
netboot/install.386/gtk/initrd.gz:
	mkdir -p netboot/install.386/gtk
	wget http://ftp.nl.debian.org/debian/dists/etch/main/installer-i386/current/images/netboot/gtk/debian-installer/i386/initrd.gz -O $@
netboot/install.amd/vmlinuz:
	mkdir -p netboot/install.amd
	wget http://ftp.nl.debian.org/debian/dists/etch/main/installer-amd64/current/images/netboot/debian-installer/amd64/linux -O $@
netboot/install.amd/initrd.gz:
	mkdir -p netboot/install.amd
	wget http://ftp.nl.debian.org/debian/dists/etch/main/installer-amd64/current/images/netboot/debian-installer/amd64/initrd.gz -O $@
netboot/install.amd/gtk/initrd.gz:
	mkdir -p netboot/install.amd/gtk
	wget http://ftp.nl.debian.org/debian/dists/etch/main/installer-amd64/current/images/netboot/gtk/debian-installer/amd64/initrd.gz -O $@

clean:
	$(MAKE) -C l10n clean
	rm -f plugins/cpuid/*.dll plugins/cpuid/*.exe plugins/systeminfo/*.dll plugins/*.dll win32-loader.exe \
		win32-loader.iso *~ */*~ license core.img g2ldr g2ldr.mbr

distclean: clean
	rm -f debian/compat
	rm -rf netboot
