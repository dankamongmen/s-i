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

# try to run a architecture script or use the common ones
export DEBIAN_FRONTEND
if [ -f /usr/share/debian-installer/partapps/$ARCH ]; then
	sh /usr/share/debian-installer/partapps/$ARCH "$DISCS"
else
	sh /usr/share/debian-installer/partapps/common "$DISCS"
fi
if [ $? -ne 0 ]; then
	db_subst $DEBCONF_BASE/fdiskerr DISC $DISCS
	db_input critical $DEBCONF_BASE/fdiskerr
	db_go
	exit 1
fi

exit 0

