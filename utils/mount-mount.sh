#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

DEBCONF_BASE="di-utils-mount-partitions"
TARGET="/target"
PREMOUNTS="/usr,/boot,/home,/var,manual"	# !!! NO SPACES !!!
SILLYMOUNTS="/etc /proc /dev"

# generate the premounts list, and hide already used
get_premounts() {
	RET=""
	for m in `echo "$PREMOUNTS" | sed -e 's/,/ /g'`; do
		[ -z "$m" ] && continue

		grep -q " ${TARGET}${m} " /proc/mounts
		[ $? -eq 0 ] && continue

		if [ -z "$RET" ]; then
			RET="$m"
		else
			RET="${RET},$m"
		fi
	done
}

#
# create now the filesystems
do_mount() {
	part="$1"
	omnt="$2"
	mnt="${TARGET}${2}"

	# create the relative-mountpoint
	mkdir -p "$mnt"
	if [ $? -ne 0 ]; then
		db_subst $DEBCONF_BASE/mkmntfailed MOUNT $mnt
		db_input high $DEBCONF_BASE/mkmntfailed
		db_go
		return 1
	fi

	# mount the device
	mount "$part" "$mnt"
	if [ $? -ne 0 ]; then
		db_subst $DEBCONF_BASE/mountfailed PART $part
		db_subst $DEBCONF_BASE/mountfailed MOUNT $mnt
		db_input high $DEBCONF_BASE/mountfailed
		db_go
		return 1
	fi

	# update fstab for baseinstaller
	sh /usr/lib/prebaseconfig.d/40fstab
	return 0
}

# collection infos from system
get_partitions
PARTITIONS="$RET"
get_premounts
PREMOUNTS="$RET"

# print a note, if no partitions are found
if [ -z "$PARTITIONS" ]; then
	db_set $DEBCONF_BASE/nopart "false"
	db_input high $DEBCONF_BASE/nopart
	db_go
	exit 1
fi

# if we have not yet mounted the root partition
grep " ${TARGET} " /proc/mounts 2>/dev/null
if [ $? -ne 0 ]; then
	db_subst $DEBCONF_BASE/rootpartition PARTITIONS $PARTITIONS
	db_set $DEBCONF_BASE/rootpartition "false"
	db_input medium $DEBCONF_BASE/rootpartition
	db_go
	db_get $DEBCONF_BASE/rootpartition
	PARTITIONS="$RET"
	MOUNTPOINT="/"
fi

# ask for mount-point (if no root mounted allow only root)
grep " ${TARGET} " /proc/mounts 2>/dev/null
if [ $? -eq 0 ]; then
	# ask for partitions
	db_subst $DEBCONF_BASE/which-partition PARTITIONS $PARTITIONS
	db_set $DEBCONF_BASE/which-partition "false"
	db_input medium $DEBCONF_BASE/which-partition
	db_go
	db_get $DEBCONF_BASE/which-partition
	PARTITIONS="$RET"

	db_subst $DEBCONF_BASE/mountpoint MOUNTPOINTS $PREMOUNTS
	db_subst $DEBCONF_BASE/mountpoint PARTITION $PARTITIONS
	db_set $DEBCONF_BASE/mountpoint "/"
	db_input medium $DEBCONF_BASE/mountpoint
	db_go
	db_get $DEBCONF_BASE/mountpoint
	MOUNTPOINT="$RET"
fi

# ask for manual mointpoint if needed (multi lang support)
echo "$MOUNTPOINT" | grep -q '^/'
if [ $? -ne 0 ]; then
	db_subst $DEBCONF_BASE/manual-mountpoint \
		PARTITION $PARTITIONS
	db_set $DEBCONF_BASE/manual-mountpoint "false"
	db_input medium $DEBCONF_BASE/manual-mountpoint
	db_go
	db_get $DEBCONF_BASE/manual-mountpoint
	MOUNTPOINT="$RET"
fi

# make simple sanity check
if [ -z "$PARTITIONS" -o -z "$MOUNTPOINT" ]; then
        db_input high $DEBCONF_BASE/nosel
        db_go
        exit 1
fi

# check, if the partition is already mounted
grep -q " ${TARGET}${MOUNTPOINT} " /proc/mounts 1>/dev/null 2>&1
if [ $? -eq 0 ]; then
	db_subst $DEBCONF_BASE/doublemnt MOUNTPOINT $MOUNTPOINT
	db_set $DEBCONF_BASE/doublemnt "false"
	db_input high $DEBCONF_BASE/doublemnt
	db_go
	exit 1
fi

# don't allow silly mount points
ALLOW=1
for m in $SILLYMOUNTS; do
	if [ "$m" = "$MOUNTPOINT" ]; then
		ALLOW=0
		break
	fi
done
if [ $ALLOW -ne 1 ]; then
	db_subst $DEBCONF_BASE/sillymnt MOUNTPOINT $MOUNTPOINT
	db_set $DEBCONF_BASE/sillymnt "false"
	db_input high $DEBCONF_BASE/sillymnt
	db_go
	exit 1
fi

# finally start mounting
do_mount $PARTITIONS $MOUNTPOINT
[ $? -ne 0 ] && exit 1

exit 0

