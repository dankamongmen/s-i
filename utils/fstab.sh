#!/bin/sh

TARGET="/target"
FSTAB="$TARGET/etc/fstab"

# make sure /etc exists
mkdir -p `dirname $FSTAB`

# print fstab header
echo "# /etc/fstab: static file system information.">$FSTAB
echo "#">>$FSTAB
echo "# <file system> <mount point>   <type>  <options>               <dump>  <pass>">>$FSTAB

# print filesystems
grep "on $TARGET" /proc/mounts | \
while read LINE; do
	set -- `echo $LINE`
	mnt=`echo $3 | sed -e "s#^$TARGET##"`
	[ -z "$mnt" ] && mnt="/"
	pass="2"
	[ "$mnt" = "/" ] && pass="1"
	echo "$1\t$mnt\t$5\tdefaults\t0\t$pass">>$FSTAB
done

# print swaps
for i in `cat /proc/swaps | grep '^/' | cut -d " " -f 1`; do
	echo "$i\t/none\tswap\tsw\t0\t0">>$FSTAB
done

# print other generic mounts
echo "proc\t/proc\tproc\tdefaults\t0\t0">>$FSTAB

exit 0

