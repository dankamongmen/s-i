#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

DEBCONF_BASE="di-utils-mkswap"
FSID="82"

#
# create now the filesystems
do_mkswap() {
	bad="$1"
	parts="$2"

	if [ "$bad" = "true" ]; then
		bad="-c"
	else
		bad=""
	fi

	for p in `echo $parts | sed -e 's/,/ /g'`; do
		/sbin/mkswap $bad $p
		if [ $? -ne 0 ]; then
			db_subst $DEBCONF_BASE/mkswaperror PARTITION $p
			db_input high $DEBCONF_BASE/mkswaperror
			db_go
			return 1
		fi
		/sbin/swapon $p
		if [ $? -ne 0 ]; then
			db_subst $DEBCONF_BASE/swaponerror PARTITION $p
			db_input high $DEBCONF_BASE/swaponerror
			db_go
			return 1
		fi
	done

        # update fstab for baseinstaller
	sh /usr/lib/prebaseconfig.d/40fstab
	return 0
}

# collection infos from system
get_partitions
PARTITIONS="$RET"

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

# should we detect bad blocks in disc?
BADBLOCK="false"
if [ -x /sbin/badblocks ]; then
	db_set $DEBCONF_BASE/badblock "false"
	db_input medium $DEBCONF_BASE/badblock
	db_go
	db_get $DEBCONF_BASE/badblock
	BADBLOCK="$RET"
fi

db_subst $DEBCONF_BASE/confirm PARTITION $PARTITIONS
db_set $DEBCONF_BASE/confirm "false"
db_input medium $DEBCONF_BASE/confirm
db_go
db_get $DEBCONF_BASE/confirm
if [ "$RET" = "true" ]; then
	do_mkswap "$BADBLOCK" "$PARTITIONS"
	[ $? -ne 0 ] && exit 1
fi

exit 0

