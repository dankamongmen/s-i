# Wipes any traces of an active MD on the given device
device_remove_md() {
	local dev md_dev md_devs part used_parts type removed_devices confirm
	dev="$1"
	cd $dev

	realdev=$(mapdevfs "$(cat $dev/device)")
	md_devs=$(sed -n -e \
		"s,^\(md[0-9]*\) : \(active raid[0-9]*\|inactive\) .*${realdev#/dev/}[^[]*\[[0-9]\].*,/dev/\1,p" \
		/proc/mdstat)

	if [ -z "$md_devs" ]; then
		return 0
	fi

	used_parts=""
	removed_devices=""
	for md_dev in $md_devs; do
		used_parts="${used_parts:+$used_parts }$(
			mdadm -Q --detail $md_dev |
			grep -E "^[[:space:]]*[0-9].*(active|spare)" |
			sed -e 's/.* //')"
		type="$(mdadm -Q --detail $md_dev |
			sed -n -e 's/^.*Raid Level : \(.*\)/\1/p')"
		removed_devices="${removed_devices:+$removed_devices, }${md_dev#/dev/} ($type)"
	done

	db_fget partman-md/device_remove_md seen
	if [ $RET = true ]; then
		db_get partman-md/device_remove_md
		confirm=$RET
	else
		db_subst partman-md/device_remove_md REMOVED_DEVICES "$removed_devices"
		db_subst partman-md/device_remove_md REMOVED_PARTITIONS \
			"$(echo $used_parts | sed -e 's/ /, /')"
		db_input critical partman-md/device_remove_md
		db_go || return 1
		db_get partman-md/device_remove_md
		confirm=$RET
		db_reset partman-md/device_remove_md
	fi
	if [ "$confirm" != true ]; then
		return 1
	fi

	if [ -e /lib/partman/lib/lvm-remove.sh ]; then
		. /lib/partman/lib/lvm-remove.sh
		for md_dev in $md_devs; do
			device_remove_lvm "$md_dev" || return 1
		done
	fi
	for md_dev in $md_devs; do
		logger -t md-remove "Removing $md_dev ($used_parts)"
		log-output -t md-remove mdadm --stop $md_dev || return 1
	done
	for part in $used_parts; do
		log-output -t md-remove mdadm \
			--zero-superblock --force $part || return 1
	done

	# Restart partman to get rid of stale informations
	stop_parted_server
	restart_partman || return 1

	return 0
}
