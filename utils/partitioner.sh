#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

DEBCONF_BASE="di-utils-partitioner"

# read all discs from system
get_discs
DISCS="$RET"
if [ -z "$DISCS" ]; then
	db_input critical $DEBCONF_BASE/nodiscs
	db_go
	exit 1
fi

# ask the user which disc should be partitioned
db_subst $DEBCONF_BASE/disc DISCS $DISCS
db_input critical $DEBCONF_BASE/disc
db_go
db_get $DEBCONF_BASE/disc
DISCS="$RET"

# which fdisk version should we use?
FDISK="fdisk"
[ "$DEBIAN_FRONTEND" = "slang" ] && FDISK="cfdisk"
# handle different on s390 :)
[ "$ARCH" = "s390" -o "$ARCH" = "s390x" ] && FDISK="fdasd"

# finally start the fdisk program
$FDISK $DISCS < /dev/console >/dev/console 2>/dev/console
if [ $? -ne 0 ]; then
	db_subst $DEBCONF_BASE/fdiskerr DISC $DISCS
	db_input critical $DEBCONF_BASE/fdiskerr
	db_go
	exit 1
fi

exit 0

