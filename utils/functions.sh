#
# di-utils core functions
#

# define some generics
ARCH=`udpkg --print-architecture`
FSIGNORE="(minix|msdos|vfat)"

#
# return all available physical harddrives
#
get_discs() {
	RET=""

	for d in `ls -l /dev/discs/* | sed -e 's/.*[[:space:]]\(.*\)$/\1/' -e 's,^\.\.,/dev,' -e 's,$,/disc,'`; do
		if [ -z "$RET" ]; then
			RET="$d"
		else
			RET="$RET, $d"
		fi
	done

	return 0
}

#
# return all available partitions/volumes in system
#
get_partitions() {
	RET=""

	for p in `grep -v '^major' /proc/partitions | \
	   sed -e 's/ \+/ /g' | cut -d " " -f 5 | sort`; do
		# ignore whole disc
		echo $p | grep -q 'disc$'
		[ $? -eq 0 ] && continue

		# workaround for kernel bug with ide-cdrom's
		echo "$p" | grep -q '^hd[a-z]'
		[ $? -eq 0 ] && continue

		# check partition type
		check_type "/dev/$p"
		[ $? -ne 0 ] && continue
		# check override files
		check_override "/dev/$p"
		[ $? -ne 0 ] && continue
		# check mount/swapon
		if [ "$FSID" = "82" ]; then
			grep -q "^/dev/$p[^0-9].*partition" /proc/swaps
		else
			grep -q "^/dev/$p[^0-9]" /proc/mounts
		fi
		[ $? -eq 0 ] && continue

		if [ -z "$RET" ]; then
			RET="/dev/$p"
		else
			RET="${RET},/dev/$p"
		fi
	done

	return 0
}

#
# get all available filesystem (globbing mkfs.*)
#
get_filesystems() {
	RET=""

	for f in `ls -1 /sbin/mkfs.* 2>/dev/null | sort`; do
		str=`echo $f | sed -e 's/.*\.\(.*\)$/\1/'`
		[ -z "$str" ] && continue

		# skip IGNORED fs
		echo $str | grep -q "^${FSIGNORE}$"
		[ $? -eq 0 ] && continue

		check_fs "$str"
		[ $? -ne 0 ] && continue

		if [ -z "$RET" ]; then
			RET="$str"
		else
			RET="${RET},$str"
		fi
	done

	if [ -z "$RET" ]; then
		db_input high $DEBCONF_BASE/nofs
		db_go
		exit 1
	fi

	return 0
}

#
# check if the partition is 0x${FSID} (83,82,...)
#
check_type() {
	[ -z "$1" ] && return 1
	[ -z "$FSID" ] && FSID="83"

	disc=`echo "$1" | sed -e 's/^\(.*\)\/\(.*\)$/\1\/disc/'`

	# check if this is a real harddisc (eg scsi, ide, ...)
	found="0"

	for d in `ls -l /dev/discs/* | sed -e 's/.*[[:space:]]\(.*\)$/\1/' -e 's,^\.\.,/dev,' -e 's,$,/disc,'`; do
		[ -z "$d" ] && continue
		[ "$d" = "$disc" ] && found="1"
	done
	[ $found -eq 0 ] && return 0

	if [ "$ARCH" != "s390" ]; then
		sfdisk -d $disc 2>/dev/null | grep -q "^$1[^0-9].*Id=${FSID}"
		[ $? -ne 0 ] && return 1
	else
		fdasd -p $disc | grep "$i.*Linux native"
		[ $? -ne 0 ] && return 1
	fi

	return 0
}

#
# check if the filesystem is also supported by kernel
#
check_fs() {
	# We only list fs, which are supported by kernel
	grep -v '^nodev' /proc/filesystems | grep -q "$1"
	[ $? -eq 0 ] && return 0

	return 1
}

#
# each udeb can place a special script which check the partition
#
check_override() {
	part="$1"

	for i in `find /usr/share/debian-installer/overrides/* 2>/dev/null`; do
		[ ! -f "$i" ] && continue
		case "$i" in
		*.sh)
			sh $i $part
			;;
		*)
			$i $i $part
			;;
		esac
		[ $? -ne 0 ] && return 1
	done

	return 0
}

