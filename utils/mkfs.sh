#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

DEBCONF_BASE="di-utils-mkfs"
FSID="83"

#
# create now the filesystems
do_mkfs() {
	fs="$1"
	bad="$2"
	parts="$3"

	if [ "$bad" = "true" ]; then
		bad="-c"
	else
		bad=""
	fi

	for p in `echo $parts | sed -e 's/,/ /g'`; do
		/sbin/mkfs.$fs $bad $p
		if [ $? -ne 0 ]; then
			db_subst $DEBCONF_BASE/mkfserror FILESYSTEM $fs
			db_subst $DEBCONF_BASE/mkfserror PARTITION $p
			db_input high $DEBCONF_BASE/mkfserror
			db_go
			return 1
		fi
	done
	return 0
}

# activate ext3 if possible
modprobe ext3 1>/dev/null 2>&1

# collection infos from system
get_partitions
PARTITIONS="$RET"
get_filesystems
FS="$RET"

# ask for partitions
db_subst $DEBCONF_BASE/partitions PARTITIONS $PARTITIONS
db_set $DEBCONF_BASE/partitions "false"
db_input medium $DEBCONF_BASE/partitions
db_go
db_get $DEBCONF_BASE/partitions
PARTITIONS="$RET"

# make simple sanity check
if [ -z "$PARTITIONS" ]; then
	db_input high $DEBCONF_BASE/nosel
	db_go
	exit 1
fi

# ask for filesystem
db_subst $DEBCONF_BASE/filesystems FILESYSTEMS $FS
db_input medium $DEBCONF_BASE/filesystems
db_go
db_get $DEBCONF_BASE/filesystems
FS="$RET"

# make simple sanity check
if [ -z "$FS" ]; then
	db_input high $DEBCONF_BASE/nosel
	db_go
	exit 1
fi

# should we detect bad blocks in disc?
BADBLOCK="false"
if [ -x /sbin/badblocks ]; then
	db_set $DEBCONF_BASE/badblock "false"
	db_input medium $DEBCONF_BASE/badblock
	db_go
	db_get $DEBCONF_BASE/badblock
	BADBLOCK="$RET"
fi

# ask the user for confirming
db_subst $DEBCONF_BASE/confirm PARTITION $PARTITIONS
db_subst $DEBCONF_BASE/confirm FILESYSTEM $FS
db_set $DEBCONF_BASE/confirm "false"
db_input medium $DEBCONF_BASE/confirm
db_go
db_get $DEBCONF_BASE/confirm
if [ "$RET" = "true" ]; then
	do_mkfs "$FS" "$BADBLOCK" "$PARTITIONS"
	[ $? -ne 0 ] && exit 1
fi

exit 0

