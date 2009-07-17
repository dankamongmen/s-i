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

md_get_partitions () {
	local dev method

        PARTITIONS=""

	for dev in $DEVICES/*; do
		[ -d "$dev" ] || continue
		cd $dev
		open_dialog PARTITIONS
		while { read_line num id size type fs path name; [ "$id" ]; }; do
			[ -f $id/method ] || continue
			method=$(cat $id/method)
			if [ "$method" = raid ]; then
				local mappedpath="$(mapdevfs "$path")"
				# Exclude partitions that are already part
				# of a RAID set
				if ! egrep -q "(${path#/dev/}|${mappedpath#/dev/})" /proc/mdstat; then
					echo "$mappedpath"
				fi
			fi
		done
		close_dialog
	done
}

# Converts a list of space (or newline) separated values to comma separated values
# TODO: duplication from partman-lvm
ssv_to_csv () {
	local csv value

	csv=""
	for value in $1; do
		if [ -z "$csv" ]; then
			csv="$value"
		else
			csv="$csv, $value"
		fi
	done
	echo "$csv"
}

# Converts a list of comma separated values to space separated values
# TODO: duplication from partman-lvm
csv_to_ssv () {
	echo "$1" | sed -e 's/ *, */ /g'
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
