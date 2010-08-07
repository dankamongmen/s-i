. /lib/partman/lib/base.sh

# Translate device from /proc/mdstat (mdX) to device node
md_devnode () {
	local num=$(echo $1 | sed -e "s/^md//")

	# Also handle the case where the first does not exist
	if [ -b /dev/md/$num ]; then
		echo /dev/md/$num
	elif [ -b /dev/md$num ]; then
		echo /dev/md$num
	else
		return 1
	fi
}

# Device node to use at creation time.  This is a bit nasty as it differs
# between mdadm metadata formats, and the only way to find out the default
# seems to be to check mdadm's version (otherwise we break installation of
# older releases).
md_devnode_create () {
	if mdadm -V 2>&1 | egrep -q '^mdadm - v([012]|3\.0)'; then
		echo "/dev/md$1"
	else
		echo "/dev/md/$1"
	fi
}

# Would this partition be allowed as a RAID physical volume?
md_allowed () {
	local dev=$1
	local id=$2

	cd $dev

	# sparc can not have RAID starting at 0 or it will destroy the partition table
	if [ "$(udpkg --print-architecture)" = sparc ] && \
	   [ "${id%%-*}" = "0" ]; then
		return 1
	fi

	local md=no
	local fs
	open_dialog PARTITION_INFO $id
	read_line x1 x2 x3 x4 fs x6 x7
	close_dialog
	if [ "$fs" = free ]; then
		# parted can't deal with VALID_FLAGS on free space as yet,
		# so unfortunately we have to special-case label types.
		local label
		open_dialog GET_LABEL_TYPE
		read_line label
		close_dialog
		case $label in
		    amiga|bsd|dasd|gpt|mac|msdos|sun)
			# ... by creating a partition
			md=yes
			;;
		esac
	else
		local flag
		open_dialog VALID_FLAGS $id
		while { read_line flag; [ "$flag" ]; }; do
			if [ "$flag" = raid ]; then
				md=yes
			fi
		done
		close_dialog
	fi

	[ $md = yes ]
}

md_list_allowed () {
	partman_list_allowed md_allowed
}

md_list_allowed_free () {
	local line

	IFS="$NL"
	for line in $(md_list_allowed); do
		restore_ifs
		local dev="${line%%$TAB*}"
		local rest="${line#*$TAB}"
		local id="${rest%%$TAB*}"
		if [ -e "$dev/locked" ] || [ -e "$dev/$id/locked" ]; then
			continue
		fi
		local path="${line##*$TAB}"
		if [ ! -e "$path" ]; then
			echo "$line"
		else
			local mappedpath="$(mapdevfs "$path")"
			# Exclude partitions that are already part of a RAID
			# set
			if ! egrep -q "(${path#/dev/}|${mappedpath#/dev/})" /proc/mdstat; then
				echo "$line"
			fi
		fi
		IFS="$NL"
	done
	restore_ifs
}

# Prepare a partition for use as a RAID physical volume. If this returns
# true, then it did some work and a commit is necessary. Prints the new
# path.
md_prepare () {
	local dev="$1"
	local id="$2"
	local size parttype fs path

	cd "$dev"
	open_dialog PARTITION_INFO "$id"
	read_line x1 id size freetype fs path x7
	close_dialog

	if [ "$fs" = free ]; then
		local newtype

		case $freetype in
		    primary)
			newtype=primary
			;;
		    logical)
			newtype=logical
			;;
		    pri/log)
			local parttype
			open_dialog PARTITIONS
			while { read_line x1 x2 x3 parttype x5 x6 x7; [ "$parttype" ]; }; do
				if [ "$parttype" = primary ]; then
					has_primary=yes
				fi
			done
			close_dialog
			if [ "$has_primary" = yes ]; then
				newtype=logical
			else
				newtype=primary
			fi
			;;
		esac

		open_dialog NEW_PARTITION $newtype ext2 $id full $size
		read_line x1 id x3 x4 x5 path x7
		close_dialog
	fi

	mkdir -p "$id"
	local method="$(cat "$id/method" 2>/dev/null || true)"
	if [ "$method" = swap ]; then
		disable_swap "$dev" "$id"
	fi
	if [ "$method" != raid ]; then
		echo raid >"$id/method"
		rm -f "$id/use_filesystem"
		rm -f "$id/format"
		update_partition "$dev" "$id"
		echo "$path"
		return 0
	fi

	echo "$path"
	return 1
}

md_get_level () {
	echo $(mdadm -Q --detail $1 | grep "Raid Level" | sed "s/.*: //")
}

md_get_devices () {
	local device
	MD_DEVICES=""
	for device in $(grep ^md /proc/mdstat | \
			sed -e 's/^\(md.*\) : .*/\1/'); do
		local mddev=$(md_devnode $device) || return 1
		local mdtype=$(md_get_level $mddev)
		MD_DEVICES="${MD_DEVICES:+$MD_DEVICES, }${device}_$mdtype"
	done
}

md_db_get_number () {
	local question="$1"
	local min="$2"
	local max="$3"
	while :; do
		db_input critical "$question"
		db_go || return $?
		db_get "$question"
		# Figure out if the user entered a number
		RET="$(echo "$RET" | sed 's/[[:space:]]//g')"
		if [ "$RET" ] && [ "$RET" -ge "$min" ] && [ "$RET" -le "$max" ]; then
			break
		fi
	done
	return 0
}

# Find the next available MD device number
md_next_device_number () {
	local md_num="$(grep ^md /proc/mdstat | \
			sed -e 's/^md\(.*\) : active .*/\1/' | sort | tail -n1)"
	if [ -z "$md_num" ]; then
		md_num=0
	else
		md_num="$(($md_num + 1))"
	fi
	echo "$md_num"
}

md_lock_devices () {
	local name dev
	name="$1"
	shift

	db_subst partman-md/text/in_use DEVICE "$name"
	db_metaget partman-md/text/in_use description
	for dev; do
		partman_lock_unit "$dev" "$RET"
	done
}
