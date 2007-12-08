## Shared code for all guided partitioning components

# Wipes any traces of LVM from a disk
# Normally you wouldn't want to use this function, 
# but wipe_disk() which will also call this function.
lvm_wipe_disk() {
	local dev realdev vg pvs pv lv tmpdev restart
	dev="$1"
	cd $dev

	if [ ! -e /lib/partman/lib/lvm-base.sh ]; then
		return 0
	fi

	. /lib/partman/lib/lvm-base.sh

	# Check if the device already contains any physical volumes
	realdev=$(mapdevfs "$(cat $dev/device)")
	if ! pv_on_device "$realdev"; then
		return 0
	fi

	# Ask for permission to erase LVM volumes 
	db_input critical partman-auto/purge_lvm_from_device
	db_go || return 1
	db_get partman-auto/purge_lvm_from_device
	if [ "$RET" != true ]; then
		return 1
	fi

	# We need devicemapper support
	modprobe dm-mod >/dev/null 2>&1

	# Check all VG's
	for vg in $(vg_list); do
		pvs=$(vg_list_pvs $vg)
		
		# Only deal with VG's on the selected disk
		if ! $(echo "$pvs" | grep -q "$realdev"); then
			continue
		fi

		# Make sure the VG don't span any other disks
		if $(echo -n "$pvs" | grep -q -v "$realdev"); then
			log-output -t partman-auto-lvs vgs
			db_input critical partman-auto/cannot_purge_lvm_from_device || true
			db_go || true
			return 1
		fi

		# Remove LV's from the VG
		for lv in $(vg_list_lvs $vg); do
			lv_delete $vg $lv
		done

		# Remove the VG and its PV's 
		vg_delete $vg
		for pv in $pvs; do
			pv_delete $pv
		done
	done

	# Make sure that parted has no stale LVM info
	restart=""
	for tmpdev in $DEVICES/*; do
		[ -d "$tmpdev" ] || continue

		realdev=$(cat $tmpdev/device)

		if [ -b "$realdev" ] || \
		   ! $(echo "$realdev" | grep -q "/dev/mapper/"); then
			continue
		fi

		rm -rf $tmpdev
		restart=1
	done

	if [ "$restart" ]; then
		stop_parted_server
		restart_partman || return 1
	fi

	return 0
}

wipe_disk() {
	local dev
	dev="$1"

	lvm_wipe_disk "$dev" || return 1

	# Create new disk label; don't prompt for label
	. /lib/partman/lib/disk-label.sh
	create_new_label "$dev" no || return 1

	cd $dev

	free_space=''
	open_dialog PARTITIONS
	while { read_line num id size type fs path name; [ "$id" ]; }; do
		if [ "$fs" = free ]; then
			free_space=$id
			free_size=$size
			free_type=$type
		fi
	done
	close_dialog
}

### XXXX: I am not 100% sure if this is exactly what this code is doing.
### XXXX: rename is of course an option. Just remember to do it here, in
### XXXX: perform_recipe and in partman-auto-lvm
create_primary_partitions() {
	cd $dev

	while [ "$free_type" = pri/log ] && \
	      echo $scheme | grep '\$primary{' >/dev/null; do
		pull_primary
		set -- $primary
		open_dialog NEW_PARTITION primary $4 $free_space beginning ${1}000001
		read_line num id size type fs path name
		close_dialog
		if [ -z "$id" ]; then
			db_progress STOP
			autopartitioning_failed
		fi
		neighbour=$(partition_after $id)
		if [ "$neighbour" ]; then
			open_dialog PARTITION_INFO $neighbour
			read_line x1 new_free_space x2 new_free_type fs x3 x4
			close_dialog
		fi
		if [ -z "$neighbour" ] || [ "$fs" != free ] || \
		   [ "$new_free_type" = primary ] || \
		   [ "$new_free_type" = unusable ]; then
			open_dialog DELETE_PARTITION $id
			close_dialog
			open_dialog NEW_PARTITION primary $4 $free_space end ${1}000001
			read_line num id size type fs path name
			close_dialog
			if [ -z "$id" ]; then
				db_progress STOP
				autopartitioning_failed
			fi
			neighbour=$(partition_before $id)
			if [ "$neighbour" ]; then
				open_dialog PARTITION_INFO $neighbour
				read_line x1 new_free_space x2 new_free_type fs x3 x4
				close_dialog
			fi
			if [ -z "$neighbour" ] || [ "$fs" != free ] ||
			   [ "$new_free_type" = unusable ]; then
				open_dialog DELETE_PARTITION $id
				close_dialog
				break
			fi
		fi
		shift; shift; shift; shift
		setup_partition $id $*
		primary=''
		scheme="$logical"
		free_space=$new_free_space
		free_type="$new_free_type"
	done
}

create_partitions() {
    foreach_partition '
	if [ -z "$free_space" ]; then
		db_progress STOP
		autopartitioning_failed
	fi
	open_dialog PARTITION_INFO $free_space
	read_line x1 free_space x2 free_type fs x3 x4
	close_dialog
	if [ "$fs" != free ]; then
		free_type=unusable
	fi

	case "$free_type" in
	    primary|logical)
		type="$free_type"
		;;
	    pri/log)
		type=logical
		;;
	    unusable)
		db_progress STOP
		autopartitioning_failed
		;;
	esac

	if [ "$last" = yes ]; then
		open_dialog NEW_PARTITION $type $4 $free_space full ${1}000001
	else
		open_dialog NEW_PARTITION $type $4 $free_space beginning ${1}000001
	fi
	read_line num id size type fs path name
	close_dialog
	if [ -z "$id" ]; then
		db_progress STOP
		autopartitioning_failed
	fi

	# Mark the partition LVM only if it is actually LVM and add it to vgpath
	if echo "$*" | grep -q "method{ lvm }"; then
		devfspv_devices="$devfspv_devices $path"
		open_dialog GET_FLAGS $id
		flags=$(read_paragraph)
		close_dialog
		open_dialog SET_FLAGS $id
		write_line "$flags"
		write_line lvm
		write_line NO_MORE
		close_dialog
	fi
	shift; shift; shift; shift
	setup_partition $id $*
	free_space=$(partition_after $id)'
}

get_auto_disks() {
	local dev device

	for dev in $DEVICES/*; do
		[ -d "$dev" ] || continue

		# Skip /dev/mapper/X and /dev/mdX devices
		device=$(cat $dev/device)
		$(echo "$device" | grep -q "/dev/md[0-9]*$") && continue
		$(echo "$device" | grep -q "/dev/mapper/") && continue

		printf "$dev\t$(device_name $dev)\n"
	done
}

select_auto_disk() {
	local DEVS

	DEVS=$(get_auto_disks)
	[ -n "$DEVS" ] || return 1
	debconf_select critical partman-auto/select_disk "$DEVS" "" || return 1
	echo "$RET"
	return 0
}

# TODO: Add a select_auto_disks() function
# Note: This needs a debconf_multiselect equiv.
