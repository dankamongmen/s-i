#!/bin/sh

TARGET="/target"
FSTAB="$TARGET/etc/fstab"

# make sure /etc exists
mkdir -p `dirname $FSTAB`

(
	# print fstab header
    	echo "# /etc/fstab: static file system information."
	echo "#"
        echo "# <file system>	<mount point>	<type>	<options>	<dump>	<pass>"
	# print filesystems
	grep " $TARGET" /proc/mounts | \
		while read LINE; do
		set -- `echo $LINE`
		devpath=`mapdevfs $1` || devpath=$1
		mnt=$2
		fstype=$3
		mnt=`echo $mnt | sed -e "s#^$TARGET##"`
		[ -z "$mnt" ] && mnt="/"
		pass="2"
		[ "$mnt" = "/" ] && pass="1"
		echo "$devpath	$mnt	$fstype	defaults	0	$pass"
	done
	
	# print swaps
	for i in `cat /proc/swaps | grep '^/' | cut -d " " -f 1`; do
		devpath=`mapdevfs $i` || devpath=$i
		echo "$devpath	none	swap	sw		0	0"
	done

	# print other generic mounts
	echo "proc		/proc	proc	defaults	0	0"

	# Print floppy
	devpath=`mapdevfs /dev/floppy/0` || devpath=/dev/fd0
	echo "$devpath	/floppy	auto	rw,user,noauto	0	0"

	# Print CDROM
	echo "/dev/cdrom	/cdrom	iso9660	ro,user,noauto	0	0"
) > $FSTAB

exit 0
