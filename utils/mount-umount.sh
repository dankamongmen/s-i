#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

DEBCONF_BASE="di-utils-mount-partitions"
TARGET="/target"
PREMOUNTS="/usr,/boot,/home,/var,manual"	# !!! NO SPACES !!!
SILLYMOUNTS="/etc /proc /dev"

# generate the premounts list, and hide already used
get_mounts() {
	RET=""
	for m in `grep $TARGET /proc/mounts | cut -d " " -f 1`; do
		[ -z "$m" ] && continue

		line=`grep "^$m" /proc/mounts`
		echo "Line: $line" >>/var/log/messages
		format=`echo "$line" | cut -d " " -f 1-2 | \
			sed -e 's,^\(.*\) \(.*\),\1 (\2),' \
				-e "s,[[:space:]]($TARGET, (," \
				-e 's,(),(/),'`

		if [ -z "$RET" ]; then
			RET="$format"
		else
			RET="${RET},$format"
		fi
	done
}

# collection infos from system
get_mounts
PARTITIONS="$RET"
echo "Current mounts: $RET" >>/var/log/messages

# print a note, if no partitions are found
if [ -z "$PARTITIONS" ]; then
	db_set $DEBCONF_BASE/nompart "false"
	db_input high $DEBCONF_BASE/nompart
	db_go
	exit 1
fi

# ask for partitions
db_subst $DEBCONF_BASE/umount-partition PARTITIONS $PARTITIONS
db_set $DEBCONF_BASE/umount-partition "false"
db_input medium $DEBCONF_BASE/umount-partition
db_go
db_get $DEBCONF_BASE/umount-partition
PARTITIONS="$RET"

# finally start mounting
PART=`echo "$PARTITIONS" | cut -d " " -f 1`
MNT=`echo "$PARTITIONS" | sed -e 's/.*(\(.*\)).*/\1/'`
umount $PART
if [ $? -ne 0 ]; then
	db_subst $DEBCONF_BASE/umounterr PARTITION $PART
	db_subst $DEBCONF_BASE/umounterr MOUNTPOINT $MNT
	db_set $DEBCONF_BASE/umounterr "false"
	db_input high $DEBCONF_BASE/umounterr
	db_go
	exit 1
fi

sh /usr/lib/prebaseconfig.d/40fstab
exit 0

