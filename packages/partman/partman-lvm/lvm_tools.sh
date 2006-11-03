. /lib/partman/definitions.sh

# Avoid warnings from lvm2 tools about open file descriptors
# Setting this here should avoid them for partman as a whole
export LVM_SUPPRESS_FD_WARNINGS=1

###############################################################################
#                                
# Miscellaneous utility functions
#
###############################################################################

# Convert common terms for disk sizes into something LVM understands.
#  e.g. "200 gb" -> "200G"
lvm_size_from_human() {
	echo "$1" | tr "kmgt" "KMGT" | sed -e 's/[[:space:]]//g;s/\([KMGT]\)[bB]/\1/'
}

# Convert LVM disk sizes into something human readable.
#  e.g. "812.15M" -> "812MB"
lvm_size_to_human() {
	echo "${1}B" | sed -e 's/\...//'
}

# Convenience wrapper for lvs/pvs/vgs
lvm_get_info() {
	local type info device output
	type=$1
	info=$2
	device=$3

	output=$($type --noheadings --nosuffix --separator ":" --units M \
		-o "$info" $device 2> /dev/null)
	if [ $? -ne 0 ]; then
		return 1
	fi
	# NOTE: The last sed, s/:$// is necessary due to a bug in lvs which adds a
	#       trailing separator even if there is only one field
	output=$(echo "$output" | sed -e 's/^[:[:space:]]\+//g;s/[:[:space:]]\+$//g')
	# Be careful here, we don't want to output only a newline
	if [ -n "$output" ]; then
		echo "$output"
	fi
	return 0
}

# Converts a list of space (or newline) separated values to comma separated values
ssv_to_csv() {
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
csv_to_ssv() {
	echo "$1" | sed -e 's/ *, */ /g'
}

# Produces a human readable description of the current LVM config
lvm_get_config() {
	local output pv pvs vg vgs lv lvs line

	# Unallocated PVs
	db_metaget partman-lvm/text/configuration_freepvs description
	output="$RET
"
	pvs=$(pv_list_free)
	if [ -z "$pvs" ]; then
		db_metaget partman-lvm/text/configuration_none_pvs description
		output="$output  * $RET
"
	else
		for pv in $(pv_list_free); do
			pv_get_info "$pv"
			line=$(printf "%-56s (%sMB)" "  * $pv" "$SIZE")
			output="${output}${line}
"
		done
	fi

	# Volume groups
	db_metaget partman-lvm/text/configuration_vgs description
	output="$output
$RET
"
	vgs=$(vg_list)
	if [ -z "$vgs" ]; then
		db_metaget partman-lvm/text/configuration_none_vgs description
		RET="$output  * $RET
"
		return 0
	fi

	for vg in $vgs; do
		# VG name
		vg_get_info "$vg"
		line=$(printf "%-56s (%sMB)" "  * $vg" "$SIZE")
		output="${output}${line}
"

		# PVs used by VG
		# This will return > 0 results, otherwise we'd have no VG
		pvs=$(vg_list_pvs "$vg")
		db_metaget partman-lvm/text/configuration_pv description
		for pv in $pvs; do
			pv_get_info "$pv"
			line=$(printf "%-35s %-20s (%sMB)" "    - $RET" "$pv" "$SIZE")
			output="${output}${line}
"
		done

		# LVs provided by VG
		lvs=$(vg_list_lvs "$vg")
		if [ -z "$lvs" ]; then
			continue
		fi
		db_metaget partman-lvm/text/configuration_lv description
		for lv in $lvs; do
			lv_get_info "$vg" "$lv"
			line=$(printf "%-35s %-20s (%sMB)" "    - $RET" "$lv" "$SIZE")
			output="${output}${line}
"
		done
	done
	RET="$output"
	return 0
}	

# Common checks for VG and LV names
# Rules:
# 1) At least one character
# 2) Only alphanumeric characters (isalnum()) and "._-+"
# 3) May not be "." or ".."
# 4) must not start with a hyphen
# 5) maximum name length 128 characters
# See lvm2 source and bug #254630 for details
lvm_name_ok() {
	local name
	name="$1"

	# Rule 1
	if [ -z "$name" ]; then
		return 1
	fi

	# Rule 2
	if [ "$(echo -n "$name" | sed 's/[^-+_\.[:alnum:]]//g')" != "$name" ]; then
		return 1
	fi

	# Rule 3
	if [ "$name" = "." -o "$name" = ".." ]; then
		return 1
	fi

	# Rule 4
	if [ "$(echo -n "$name" | sed 's/^-//')" != "$name" ]; then
		return 1
	fi

	# Rule 5
	if [ $(echo -n "$name" | wc -c) -gt 128 ]; then
		return 1
	fi

	return 0
}

###############################################################################
#                                
# Physical Volume utility functions
#
###############################################################################

# Check if a device contains PVs
pv_on_device() {
	local device
	device="$1"

	if $(pvs --noheadings --nosuffix -o pv_name | grep -q "$device"); then
		return 0
	fi
	return 1
}

# Get info on a PV
pv_get_info() {
	local info

	info=$(lvm_get_info pvs pv_size,pv_pe_count,pv_free,pv_pe_alloc_count,vg_name "$1")
	if [ $? -ne 0 ]; then
		return 1
	fi

	SIZE=$(echo "$info"   | cut -d':' -f1 | cut -d'.' -f1)
	SIZEPE=$(echo "$info" | cut -d':' -f2)
	FREE=$(echo "$info"   | cut -d':' -f3 | cut -d'.' -f1)
	FREEPE=$(echo "$info" | cut -d':' -f4) # Used, not free, PEs
	FREEPE=$(( $SIZEPE - $FREEPE ))        # Calculate free PEs
	VG=$(echo "$info"     | cut -d':' -f5)
	return 0
}

# Get all PVs
pv_list() {
	# Scan the partman devices and find partitions that have lvm as method.
	# Do not rely on partition flags since it doesn't work for some partitions
	# (e.g. dm-crypt, RAID)
	local dev method

	for dev in $DEVICES/*; do
		[ -d "$dev" ] || continue
		cd $dev
		open_dialog PARTITIONS
		while { read_line num id size type fs path name; [ "$id" ]; }; do
			[ -f $id/method ] || continue
			method=$(cat $id/method)
			if [ "$method" = lvm ]; then
				echo $(mapdevfs $path)
			fi
		done
		close_dialog
	done
}

# Get all unused PVs
pv_list_free() {
	local pv vg

	for pv in $(pv_list); do
		vg=$(lvm_get_info pvs vg_name "$pv")
		if [ -z "$vg" ]; then
			echo "$pv"
		fi
	done
}

# Initialize a PV
pv_create() {
	local pv
	pv="$1"

	if pvs "$pv" > /dev/null 2>&1; then
		return 0
	fi

	log-output -t partman-lvm pvcreate -ff -y "$pv"
	return $?
}

# Remove the LVM signatures from a PV
pv_delete() {
	local pv
	pv="$1"

	if ! pvs "$pv" > /dev/null 2>&1; then
		return 0
	fi

	log-output -t partman-lvm pvremove -ff -y "$pv"
	return $?
}

###############################################################################
#                                
# Logical Volume utility functions
#
###############################################################################

# Get LV info
lv_get_info() {
	local info vg lv line tmplv
	vg=$1
	lv=$2
	info=$(lvm_get_info lvs lv_name,lv_size "$vg")

	SIZE=""
	FS="unknown"
	MOUNT="unknown"
	for line in $(lvm_get_info lvs lv_name,lv_size "$vg"); do
		tmplv=$(echo "$line" | cut -d':' -f1)
		if [ $tmplv != $lv ]; then
			continue
		fi

		SIZE=$(echo "$line" | cut -d':' -f2 | cut -d'.' -f1)
		MOUNT=$(grep "^/dev/mapper/$vg-$lv" /proc/mounts | cut -d' ' -f2 | sed -e 's/\/target//')
		# FIXME: Get FS
		#FS=$(parted "$tmplv" print | grep '^1' | \
		#	sed -e 's/ \+/ /g' | cut -d " " -f 4)
		break
	done
}

# List all LVs and their VGs
lv_list() {
	lvm_get_info lvs lv_name,vg_name ""
}

# Create a LV
lv_create() {
	local vg lv size
	vg="$1"
	lv="$2"
	size="$3"

	if [ "$size" = "full" ]; then
		vg_get_info "$vg"
		if [ "$FREEPE" -lt 1 ]; then
			return 1
		fi
		log-output -t partman-lvm lvcreate -l "$FREEPE" -n "$lv" "$vg"
	else
		log-output -t partman-lvm lvcreate -L "$size" -n "$lv" $vg
	fi
	return $?
}

# Delete a LV
lv_delete() {
	local vg lv device
	vg="$1"
	lv="$2"
	device="/dev/$vg/$lv"

	swapoff $device > /dev/null 2>&1
	umount $device > /dev/null 2>&1

	log-output -t partman-lvm lvremove -f "$device"
	return $?
}

# Checks that a logical volume name is ok
# Rules:
# 1) The common rules (see lvm_name_ok)
# 2) must not start with "snapshot"
# See lvm2 source and bug #254630 for details
lv_name_ok() {
	local lvname
	lvname="$1"

	# Rule 1
	lvm_name_ok "$lvname" || return 1

	# Rule 2
	if [ "${lvname#snapshot}" != "$lvname" ]; then
		return 1
	fi

	return 0
}

###############################################################################
#                                
# Volume Group utility functions
#
###############################################################################

# Get VG info
vg_get_info() {
	local info

	info=$(lvm_get_info vgs vg_size,vg_extent_count,vg_free,vg_free_count,lv_count,pv_count "$1")
	if [ $? -ne 0 ]; then
		return 1
	fi

	SIZE=$(echo "$info"   | cut -d':' -f1 | cut -d'.' -f1)
	SIZEPE=$(echo "$info" | cut -d':' -f2)
	FREE=$(echo "$info"   | cut -d':' -f3 | cut -d'.' -f1)
	FREEPE=$(echo "$info" | cut -d':' -f4)
	LVS=$(echo "$info"    | cut -d':' -f5)
	PVS=$(echo "$info"    | cut -d':' -f6)
	return 0
}

# List all VGs
vg_list() {
	lvm_get_info vgs vg_name ""
}

# List all VGs with free space
vg_list_free() {
	local vg

	for vg in $(vg_list); do
		vg_get_info "$vg"
		if [ $FREEPE -gt 0 ]; then
			echo "$vg"
		fi
	done
}

# Get all PVs from a VG
vg_list_pvs() {
	local line vg pv

	# vgs doesn't work with pv_name
	for line in $(lvm_get_info pvs vg_name,pv_name ""); do
		vg=$(echo "$line" | cut -d':' -f1)
		pv=$(echo "$line" | cut -d':' -f2)
		if [ "$vg" = "$1" ]; then
			echo "$pv"
		fi
	done
}

# Get all LVs from a VG
vg_list_lvs() {
	lvm_get_info lvs lv_name "$1"
}

# Create a volume group
vg_create() {
	local vg pv
	vg="$1"

	shift
	for pv in $*; do
		pv_create "$pv" || return 1
	done
	log-output -t partman-lvm vgcreate "$vg" $* || return 1
	return 0
}

# Delete a volume group
vg_delete() {
	local vg
	vg="$1"

	log-output -t partman-lvm vgchange -a n "$vg" && \
	log-output -t partman-lvm vgremove "$vg" && \
	return 0

	# reactivate if deleting failed
	log-output -t partman-lvm vgchange -a y "$vg"
	return 1
}

# Extend a volume group (add a PV)
vg_extend() {
	local vg pv
	vg="$1"
	pv="$2"

	pv_create "$pv" || return 1
	log-output -t partman-lvm vgextend "$vg" "$pv" || return 1
	return 0
}

# Reduce a volume group (remove a PV)
vg_reduce() {
	local vg pv
	vg="$1"
	pv="$2"

	log-output -t partman-lvm vgreduce "$vg" "$pv"
	return $?
}

# Checks that a logical volume name is ok
# Rules:
# 1) The common rules (see lvm_name_ok)
# See lvm2 source and bug #254630 for details
vg_name_ok() {
	local vgname
	vgname="$1"

	# Rule 1
	lvm_name_ok "$vgname" || return 1

	return 0
}
