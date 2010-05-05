
export SHELL := bash

PACKAGE	:= win32-loader
VERSION	:= $(shell head -n 1 debian/changelog | sed -e "s/^$(PACKAGE) (\(.*\)).*/\1/g")

NSIS_CC		:= i586-mingw32msvc-gcc -Os
NSIS_STRIP	:= i586-mingw32msvc-strip
NSIS_CFLAGS	:= -Wl,--file-alignment,512 -Werror -D_WIN32_WINNT=0x0500

ifeq ($(wildcard /usr/i586-mingw32msvc/include/exdll.h), /usr/i586-mingw32msvc/include/exdll.h)
NSIS_CFLAGS	+= -DHAVE_EXDLL_H
else
NSIS_CFLAGS	+= -L/usr/i586-mingw32msvc/lib/nsis -lpluginapi
endif

MAKENSIS	:= makensis -O/dev/null
# Standalone version : Not linked to a website with preseed.cfg
ifdef STANDALONE
MAKENSIS	+= -DNOCD=yes
endif

# Network version : Linked to a website with preseed.cfg
ifdef NETWORK_BASE_URL
MAKENSIS	+= -DNETWORK_BASE_URL=$(NETWORK_BASE_URL) -DNOCD=yes
endif

# hard disk
GRUB_MODULES	+= biosdisk
# partmap
GRUB_MODULES	+= part_msdos part_gpt
# fs
GRUB_MODULES	+= fat ntfs ntfscomp
# used in our grub.cfg
GRUB_MODULES	+= search linux bsd vbe boot
# might be useful for debugging
GRUB_MODULES	+= minicmd cat cpuid chain halt help ls reboot

all: win32-loader.exe g2ldr g2ldr.mbr

core.img:
	grub-mkimage -o $@ --prefix /win32-loader $(GRUB_MODULES)

g2ldr: /usr/lib/grub/i386-pc/g2hdr.bin core.img
	cat $^ > $@

g2ldr.mbr: /usr/lib/grub/i386-pc/g2ldr.mbr
	cp $^ $@

loadlin.pif: genpif
	bash $^ > $@

loadlin.exe: /usr/lib/loadlin/loadlin.exe.gz
	gunzip -c $^ > $@

win32-loader.exe: main.nsi maps.ini \
		templates/binary_choice.ini templates/graphics.ini templates/custom.ini templates/4_choices.ini \
		plugins/cpuid/test64.dll plugins/systeminfo/systeminfo.dll plugins/string.dll \
		swirl.ico license loadlin.pif loadlin.exe g2ldr g2ldr.mbr
	$(MAKE) -C l10n
	$(MAKENSIS) main.nsi
	du -h win32-loader.exe

license: license.in debian/changelog
	sed -e "s/@VERSION@/$(VERSION)/g" < $< > $@

plugins/cpuid/test64.dll: plugins/cpuid/cpuid.c plugins/cpuid/plugin.c
	$(NSIS_CC) $^ $(NSIS_CFLAGS) -shared -o $@
	$(NSIS_STRIP) $@

plugins/systeminfo/systeminfo.dll: plugins/systeminfo/systeminfo.c
	$(NSIS_CC) $^ $(NSIS_CFLAGS) -shared -o $@
	$(NSIS_STRIP) $@

plugins/string.dll: plugins/string.c
	$(NSIS_CC) $^ $(NSIS_CFLAGS) -shared -o $@
	$(NSIS_STRIP) $@

iso: stable.iso daily.iso
stable.iso: \
  netboot/download-stable-stamp \
  netboot/stable/win32-loader.exe netboot/stable/g2ldr netboot/stable/g2ldr.mbr \
  netboot/stable/autorun.inf netboot/stable/win32-loader.ini \
  $(NULL)
	genisoimage -r -J -o $@ netboot/stable

daily.iso: \
  netboot/download-daily-stamp \
  netboot/daily/win32-loader.exe netboot/daily/g2ldr netboot/daily/g2ldr.mbr \
  netboot/daily/autorun.inf netboot/daily/win32-loader.ini \
  $(NULL)
	genisoimage -r -J -o $@ netboot/daily

BASE_URL=http://ftp.nl.debian.org/debian/dists/stable/main
netboot/download-stable-stamp:
	mkdir -p netboot/stable/install.{386,amd}/gtk
	wget $(BASE_URL)/installer-i386/current/images/netboot/debian-installer/i386/linux \
		-O netboot/stable/install.386/vmlinuz
	wget $(BASE_URL)/installer-i386/current/images/netboot/debian-installer/i386/initrd.gz \
		-O netboot/stable/install.386/initrd.gz
	wget $(BASE_URL)/installer-i386/current/images/netboot/gtk/debian-installer/i386/initrd.gz \
		-O netboot/stable/install.386/gtk/initrd.gz
	wget $(BASE_URL)/installer-amd64/current/images/netboot/debian-installer/amd64/linux \
		-O netboot/stable/install.amd/vmlinuz
	wget $(BASE_URL)/installer-amd64/current/images/netboot/debian-installer/amd64/initrd.gz \
		-O netboot/stable/install.amd/initrd.gz
	wget $(BASE_URL)/installer-amd64/current/images/netboot/gtk/debian-installer/amd64/initrd.gz \
		-O netboot/stable/install.amd/gtk/initrd.gz
	touch $@

netboot/download-daily-stamp:
	mkdir -p netboot/daily/install.{386,amd}/gtk
	wget http://d-i.debian.org/daily-images/i386/daily/netboot/debian-installer/i386/linux \
		-O netboot/daily/install.386/vmlinuz
	wget http://d-i.debian.org/daily-images/i386/daily/netboot/debian-installer/i386/initrd.gz \
		-O netboot/daily/install.386/initrd.gz
	wget http://d-i.debian.org/daily-images/i386/daily/netboot/gtk/debian-installer/i386/initrd.gz \
		-O netboot/daily/install.386/gtk/initrd.gz
	wget http://d-i.debian.org/daily-images/amd64/daily/netboot/debian-installer/amd64/linux \
		-O netboot/daily/install.amd/vmlinuz
	wget http://d-i.debian.org/daily-images/amd64/daily/netboot/debian-installer/amd64/initrd.gz \
		-O netboot/daily/install.amd/initrd.gz
	wget http://d-i.debian.org/daily-images/amd64/daily/netboot/gtk/debian-installer/amd64/initrd.gz \
		-O netboot/daily/install.amd/gtk/initrd.gz
	touch $@

netboot/stable/autorun.inf netboot/daily/autorun.inf: autorun.inf
	mkdir -p netboot/{stable,daily}
	todos < $< > $@
netboot/stable/win32-loader.ini netboot/daily/win32-loader.ini: win32-loader.ini
	mkdir -p netboot/{stable,daily}
	todos < $< > $@

netboot/stable/win32-loader.exe netboot/daily/win32-loader.exe: win32-loader.exe
	mkdir -p netboot/{stable,daily}
	cp $^ $@

netboot/stable/% netboot/daily/%: %
	mkdir -p netboot/{stable,daily}
	cp $(shell basename $@) $@

clean:
	$(MAKE) -C l10n clean
	rm -f plugins/cpuid/*.dll plugins/cpuid/*.exe plugins/systeminfo/*.dll plugins/*.dll win32-loader.exe \
		*.iso *~ */*~ license core.img g2ldr g2ldr.mbr loadlin.pif loadlin.exe

distclean: clean
	rm -rf netboot
