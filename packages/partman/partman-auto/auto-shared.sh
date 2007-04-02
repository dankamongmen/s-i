## this file contains a bunch of shared code between partman-auto
## and partman-auto-lvm.

# Gets the major and minor device numbers of a device passed as $1
# and puts them in $MAJOR and $MINOR.
dev_get_major_minor() {
	local dev stats
	dev="$1"

	if [ -z "$dev" ]; then
		return 1
	fi

	stats=$(ls -al "$dev" | sed -e 's/[[:space:],]\+/ /g')
	MAJOR=$(echo "$stats" | cut -d' ' -f5)
	MINOR=$(echo "$stats" | cut -d' ' -f6)

	if [ -z "$MAJOR" ] || [ -z "$MINOR" ]; then
		return 1
	fi

	return 0
}

# Makes sure that parted has no stale info
dm_update_parted() {
	local restart tmpdev tmppart realdev

	restart="0"
	for tmpdev in $DEVICES/*; do
		for tmppart in $tmpdev/*; do
			if [ ! -d "$tmppart" ]; then
				continue
			fi

			if [ ! -e "$tmppart/crypt_active" ]; then
				continue
			fi

			if [ ! -e "$(cat "$tmppart/crypt_active")" ]; then
				rm -f "$tmppart/crypt_active"
				rm -f "$tmppart/locked"
				restart="1"
			fi
		done

		realdev=$(cat $tmpdev/device)
		if ! $(echo "$realdev" | grep -q "/dev/mapper/"); then
			continue
		fi

		if [ -b "$realdev" ]; then
			continue
		fi

		rm -rf $tmpdev
		restart="1"
	done

	if [ $restart ]; then
		stop_parted_server
		restart_partman || return 1
	fi

	return 0
}

# Given a lvm device (excluding the /dev/mapper/ part), will gather data on
# the LVM config, prompt the user and then remove the VG along with all the
# LV's and PV's which belong to that VG.
dm_wipe_lvm() {
	local dev vg pv pvs lv lvs vgtext lvtext pvtext
	dev="$1"

	# First check that the device exists, it's not an error if it doesn't
	# since one call to this function can wipe several LV's
	if [ ! -b "/dev/mapper/$dev" ]; then
		return 0
	fi

	if [ ! -e /lib/partman/lvm_tools.sh ]; then
		return 1
	fi

	. /lib/partman/lvm_tools.sh

	# We get a device like mainvg-swaplv as $1, let's figure out the VG

	# Make sure that the device includes at least one dash
        if [ "$(echo -n "$dev" | tr -d -)" = "$dev" ]; then
                return 1
	fi

        # Split volume group from logical volume.
        vg=$(echo ${dev} | sed -e 's#\(.*\)\([^-]\)-[^-].*#\1\2#')
        # Reduce padded --'s to -'s
        vg=$(echo ${vg} | sed -e 's#--#-#g')
	if [ -z "$vg" ]; then
		return 1
	fi
	pvs=$(vg_list_pvs $vg)
	lvs=$(vg_list_lvs $vg)

	# Ask for permission to erase LVM volumes 
	vgtext="$vg"
	pvtext=""
	for pv in $pvs; do
		if [ -z "$pvtext" ]; then
			pvtext="$pv"
		else
			pvtext="$pvtext, $pv"
		fi
	done
	lvtext=""
	for lv in $lvs; do
		if [ -z "$lvtext" ]; then
			lvtext="$lv"
		else
			lvtext="$lvtext, $lv"
		fi
	done

	db_subst partman-auto/purge_lvm_from_device VGTARGETS "$vgtext"
	db_subst partman-auto/purge_lvm_from_device LVTARGETS "$lvtext"
	db_subst partman-auto/purge_lvm_from_device PVTARGETS "$pvtext"
	db_input critical partman-auto/purge_lvm_from_device
	db_go || return 1
	db_get partman-auto/purge_lvm_from_device
	if [ "$RET" != "true" ]; then
		return 1
	fi

	# Remove the LV's
	for lv in $lvs; do
		lv_delete $vg $lv
	done

	# Remove the VG
	vg_delete $vg

	# Remove the PV's
	for pv in $pvs; do
		pv_delete $pv
		partman_unlock_unit $pv
	done

	return 0
}

# Given a cryptodev (minus the /dev/mapper/ part), will prompt the user and
# then remove the device.
dm_wipe_crypto() {
	local dev
	dev="$1"

	# Ask for permission to erase crypto volumes 
	db_subst partman-auto/purge_crypto_from_device TARGET "$dev"
	db_input critical partman-auto/purge_crypto_from_device
	db_go || return 1
	db_get partman-auto/purge_crypto_from_device
	if [ "$RET" != "true" ]; then
		return 1
	fi

	dmsetup remove "$dev"
	return $?
}

# Given a dm device (minus the /dev/mapper/ part), will recursively remove
# all devices which depend on it and then the device itself.
dm_wipe_dev() {
	local dev deps dep name type
	dev="$1"

	if [ ! -e "/dev/mapper/$dev" ]; then
		return 0
	fi

	dev_get_major_minor "/dev/mapper/$dev" || return 1
	deps=$(dmsetup deps | grep "($MAJOR, $MINOR)" | cut -d: -f1)
	for dep in $deps; do
		if [ ! -e "/dev/mapper/$dep" ]; then
			continue
		fi

		dev_get_major_minor "/dev/mapper/$dep" || return 1
		name=$(dmsetup info -j$MAJOR -m$MINOR | grep "Name:" | \
			cut -d: -f2 | sed -e 's/[[:space:]]//g')
		if [ -z "$name" ]; then
			return 1
		fi

		dm_wipe_dev "$name" || return 1
	done

	type=$(dmsetup table | grep "^$dev:" | cut -d' ' -f4)
	if [ -z "$type" ]; then
		return 1
	elif [ "$type" = crypt ]; then
		dm_wipe_crypto "$dev" || return 1
	else
		dm_wipe_lvm "$dev" || return 1
	fi

	return 0
}

# Wipes any dm devices from a disk (LVM and/or crypto)
# Given an argument like /dev/hda, /dev/hda1 and /dev/hda2 are also wiped...
# Normally you wouldn't want to use this function, 
# but wipe_disk() which will also call this function.
dm_wipe_disk() {
	local dev realdev devs check checkdev provide provides restart tmpdev tmppart
	dev="$1"

	if [ -z "$dev" ]; then
		return 1
	fi

	realdev=$(mapdevfs "$(cat $dev/device)")
	devs=$(ls ${realdev}* 2>/dev/null)

	# Given e.g. /dev/hda we check /dev/hda, /dev/hda1, etc...
	for check in $devs; do
		checkdev=$(readlink -f "$check")
		if [ ! -e "$checkdev" ]; then
			continue
		fi

		dev_get_major_minor "$checkdev" || return 1

		provides=$(dmsetup deps | grep "($MAJOR, $MINOR)" | cut -d: -f1)

		if [ -z "$provides" ]; then
			continue
		fi

		for provide in $provides; do
			dm_wipe_dev $provide || return 1
		done
	done
	
	dm_update_parted || return 1
	return 0
}

wipe_disk() {
	local dev
	dev="$1"

	if [ -z "$dev" ]; then
		return 1
	fi

	dm_wipe_disk "$dev" || return 1

	cd $dev || return 1
	open_dialog LABEL_TYPES
	types=$(read_list)
	close_dialog

	label_type=$(default_disk_label)

	if ! expr "$types" : ".*${label_type}.*" >/dev/null; then
		label_type=msdos # most widely used
	fi
	
	# Use gpt instead of msdos disklabel for disks larger than 2TB
	if expr "$types" : ".*gpt.*" >/dev/null; then
		if [ "$label_type" = msdos ] ; then
			disksize=$(cat size)
			if $(longint_le $(human2longint 2TB) $disksize) ; then
				label_type=gpt
			fi
		fi
	fi

	if [ "$label_type" = sun ]; then
		db_input critical partman/confirm_write_new_label
		db_go || exit 0
		db_get partman/confirm_write_new_label
		if [ "$RET" = false ]; then
			db_reset partman/confirm_write_new_label
			exit 1
		fi
		db_reset partman/confirm_write_new_label
	fi
	
	open_dialog NEW_LABEL "$label_type"
	close_dialog
	
	if [ "$label_type" = sun ]; then
		# write the partition table to the disk
		disable_swap
		open_dialog COMMIT
		close_dialog
		sync
		# reread it from there
		open_dialog UNDO
		close_dialog
		enable_swap
	fi

	# Different types partition tables support different visuals.  Some
	# have partition names other don't have, some have extended and
	# logical partitions, others don't.  Hence we have to regenerate the
	# list of the visuals
	rm -f visuals

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

### XXXX: i am not 100% sure if this is exactly what this code is doing.
### XXXX: rename is of course an option. Just remember to do it here, in
### XXXX: perform_recipe and in partman-auto-lvm
create_primary_partitions() {

	cd $dev

	while
	    [ "$free_type" = pri/log ] \
	    && echo $scheme | grep '\$primary{' >/dev/null
	do
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
	    if 
		[ -z "$neighbour" -o "$fs" != free \
		  -o "$new_free_type" = primary -o "$new_free_type" = unusable ]
	    then
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
		if 
		    [ -z "$neighbour" -o "$fs" != free -o "$new_free_type" = unusable ]
		then
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
