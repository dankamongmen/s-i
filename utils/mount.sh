#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

DEBCONF_BASE="di-utils-mount-partitions"
TARGET="/target"
export DEBCONF_BASE TARGET

while [ 1 ]; do
	db_set $DEBCONF_BASE/mainmenu "false"
	db_input high $DEBCONF_BASE/mainmenu
	db_go
	db_get $DEBCONF_BASE/mainmenu
	RET=`echo "$RET" | sed -e 's,^\(\w\+\).*,\1,'`

	case "$RET" in
	"Mount")
		sh /usr/share/debian-installer/mount/mount.sh
		;;
	"Unmount")
		sh /usr/share/debian-installer/mount/umount.sh
		;;
	*)
		break
		;;
	esac
done

exit 0

