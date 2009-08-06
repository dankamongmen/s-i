#!/bin/sh

TOP=`pwd`
for p in $(grep 'create repository ' d-i.conf | sed 's/create repository //'); do 
	cd $TOP/git/$p
	git gc --aggressive
	git update-server-info
done

mkdir git/attic
mv git/autopartit git/delo-installer git/kdetect git/packages-build \
	git/partkit git/pcidetect git/selectdevice git/ppcdetect \
	git/linux-modules-di-arm-2.6 linux-kernel-di-arm-2.6 \
	git/linux-modules-nonfree-di-i386-2.6 \
	git/linux-modules-di-i386 \
	git/linux-modules-di-arm \
	git/linux-modules-di-hppa \
	git/linux-modules-di-mips \
	git/linux-modules-di-mipsel \
	git/linux-modules-di-ia64 \
	git/linux-modules-di-sparc \
	git/linux-modules-di-powerpc \
	git/linux-modules-di-s390 \
	git/linux-modules-di-m68k \
	git/linux-modules-di-alpha \
	git/niccfg baseconfig-udeb \
	git/kernel-image-di \
   git/attic/
