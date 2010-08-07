#!/bin/sh

set -e

DESTDIR=$PWD/debian/tmp/usr/lib/debian-installer/images

get_installer () {
	ARCHITECTURE=$1
	DIRECTORY=$2

	BASEDIR=$MIRROR/dists/$DISTRIBUTION/main/installer-$ARCHITECTURE/$VERSION/images

	mkdir -p $DESTDIR/$ARCHITECTURE
	cd $DESTDIR/$ARCHITECTURE

	if wget -c $BASEDIR/netboot/netboot.tar.gz -O netboot-text.tar.gz; then
		unpack_installer $ARCHITECTURE/text netboot-text.tar.gz

		if wget -c $BASEDIR/netboot/gtk/netboot.tar.gz -O netboot-gtk.tar.gz; then
			unpack_installer $ARCHITECTURE/gtk netboot-gtk.tar.gz
		else
			rm -f netboot-gtk.tar.gz
		fi
	else
		rm -f netboot-text.tar.gz

		FILES=$(wget $BASEDIR/MANIFEST -O - | awk '/netboot/ { print $1 }' | grep -v mini.iso)

		for FILE in $FILES; do
			mkdir -p $(dirname $FILE | sed -e 's|netboot/||')
			wget -c $BASEDIR/$FILE -O $(echo $FILE | sed -e 's|netboot/||')
		done
	fi
}

unpack_installer () {
	DIRECTORY=$1
	FILE=$2

	cd $DESTDIR/$(dirname $DIRECTORY)
	mkdir -p $(basename $DIRECTORY)
	tar xfz $FILE -C $(basename $DIRECTORY)
	rm -f $FILE

	cleanup_image $DESTDIR/$DIRECTORY
}

cleanup_image () {
	DIRECTORY=$1
	cd $DIRECTORY

	# amd64 i386
	if [ -e pxelinux.0 ]; then
		rm -f pxelinux.0 pxelinux.cfg
		mv debian-installer/*/* ./
		rm -rf debian-installer
	fi

	cd $OLDPWD
}

get_installer amd64 netboot
get_installer armel
get_installer hppa
get_installer i386 netboot
get_installer ia64
get_installer mips
get_installer mipsel
get_installer powerpc
get_installer sparc
