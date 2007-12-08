. /lib/partman/lib/lvm-base.sh

# Wipes any traces of LVM from a disk
# Normally you wouldn't want to use this function, 
# but wipe_disk() which will also call this function.
device_remove_lvm() {
	local dev realdev vg pvs pv lv tmpdev restart
	dev="$1"
	cd $dev

	# Check if the device already contains any physical volumes
	realdev=$(mapdevfs "$(cat $dev/device)")
	if ! pv_on_device "$realdev"; then
		return 0
	fi

	# Ask for permission to erase LVM volumes 
	db_input critical partman-lvm/device_remove_lvm
	db_go || return 1
	db_get partman-lvm/device_remove_lvm
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
			log-output -t partman-lvm vgs
			db_input critical partman-lvm/device_remove_lvm_span || true
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
