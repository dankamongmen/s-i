#!/bin/sh

if [ "$1" = partman ]; then
	# Use partman if possible.
	. /lib/partman/definitions.sh
	partman_avail=1
else
	. /usr/share/debconf/confmodule
	partman_avail=0
fi

#
# convert the return values from "xx (yy)" => "xx"
#
convert_return() {
	RET=""

	#TODO: don't need this external file here: ugly

	echo "$1" | tr ',' '\n' | \
	while read LINE; do
		part=`echo "$LINE" | cut -d " " -f1`
		echo "$part" >>/tmp/lvmcfg.tmp
	done

	[ ! -f /tmp/lvmcfg.tmp ] && return

	for i in `cat /tmp/lvmcfg.tmp`; do
		if [ -z "$RET" ]; then
			RET="$i"
		else
			RET="$RET, $i"
		fi
	done

	rm -f /tmp/lvmcfg.tmp
}

#
# Convert common terms for disk sizes into something LVM understands.
#
human2lvm() {
    input="$1"
    input=`echo "$input" | sed -e 's/ \+KB/K/'`
    input=`echo "$input" | sed -e 's/ \+MB/M/'`
    input=`echo "$input" | sed -e 's/ \+GB/G/'`
    input=`echo "$input" | sed -e 's/ \+TB/T/'`
    input=`echo "$input" | sed -e 's/ \+\(.\)/\1/'`
    echo "$input"
}

#
# return extra informations (like size, current lv) for the
# volume group
#
addinfos_pv() {
	cmdout=`pvdisplay "$1" 2>&1`

	echo "$cmdout" | grep -q 'is a new physical volume'
	if [ $? -eq 0 ]; then
		RET2=`echo "$cmdout" | sed -e 's/.*of \(.*\)$/Size: \1/'`
		RET="${RET2}"
		return
	fi

	echo "$cmdout" | grep -q '^pvdisplay'
	if [ $? -eq 0 ]; then
		RET="unknown"
		return
	fi

	RET=""
	VG=`echo "$cmdout" | grep '^[ ]*VG Name' | \
		sed -e 's/^[ ]*VG Name \+//'`
	[ "$VG" != "" ] && RET="VG: ${VG}/ "

	RET2=`echo "$cmdout" | grep '^[ ]*PV Size' | \
		sed -e 's/^[ ]*PV Size \+\(.*\)/Size: \1/' | \
		cut -d "[" -f 1 | cut -d "/" -f 1 | sed -e 's/ $//'`
	RET="${RET}${RET2}"
}

#
# return the free space of a volume group
#
getfree_vg() {
	cmdout=`vgdisplay "$1" 2>&1`
	echo "$cmdout" | grep '^[ ]*Free  PE' | sed -e 's,^.*/ ,,' | sed -e 's/ \+$//'
}

#
# return the free space of a volume group (in physical extents [PE])
#
getfreepe_vg() {
	cmdout=`vgdisplay "$1" 2>&1`
	echo "$cmdout" | grep '^[ ]*Free  PE' | sed -e 's/^.*Size \+//' | sed -e 's/ \+.*//'
}

#
# return the size of a volume group
#
getsize_vg() {
	cmdout=`vgdisplay "$1" 2>&1`
	echo "$cmdout" | grep '^[ ]*VG Size' | sed -e 's/^[ ]*VG Size \+//'
}

#
# return extra informations (like size, current lv) for the
# volume group
#
addinfos_vg() {
	cmdout=`vgdisplay "$1" 2>&1`

	RET2="Free: `getfree_vg "$1"`"
	RET="${RET2}"

	RET2="Size: `getsize_vg "$1"`"
	RET="${RET}/ ${RET2}"

	RET2=`echo "$cmdout" | grep '^[ ]*Cur LV' | \
		sed -e 's/^[ ]*Cur LV \+\(.*\)/LVs: \1/'`
	RET="${RET}/ ${RET2}"

	RET2=`echo "$cmdout" | grep '^[ ]*Cur PV' | \
		sed -e 's/^[ ]*Cur PV \+\(.*\)/PVs: \1/'`
	RET="${RET}/ ${RET2}"
}

#
# return extra informations (like size, fs type) for the
# logical volume
#
addinfos_lv() {
	cmdout=`lvdisplay "$1" 2>&1`

	RET2=`echo "$cmdout" | grep '^[ ]*LV Size' | \
		sed -e 's/^[ ]*LV Size \+\(.*\)/Size: \1/'`
	RET="${RET2}"

	cmdout=`parted $1 print | grep '^1' | \
		sed -e 's/ \+/ /g' | cut -d " " -f 4`
	[ -z "$cmdout" ] && cmdout="unknown"
	RET="${RET}/ FS: ${cmdout}"

	cmdout=`grep "^$1" /proc/mounts | cut -d " " -f2 | \
		sed -e 's,/target,,'`
	[ -z "$cmdout" ] && cmdout="n/a"
	RET="${RET}/ Mount: ${cmdout}"
}

# get all available physical volumes
enum_pvs() {
	if [ ! "$partman_avail" ]; then
		# Not in partman, so use partconf's find-partitions to find
		# partitions marked as lvm volumes.
		/usr/lib/partconf/find-partitions --ignore-fstype 2>/dev/null | grep "[[:space:]]LVM[[:space:]]" | cut -f1
	else
		# In partman, so scan the partman devices and find
		# partitions that have their method set to lvm. We don't
		# rely on the partition flags since that does not work for
		# raid partitions.
		for dev in $DEVICES/*; do
			[ -d "$dev" ] || continue
			cd $dev
			open_dialog PARTITIONS
			while { read_line num id size type fs path name; [ "$id" ]; }; do
				[ -f $id/method ] || continue
				method=$(cat $id/method)
				if [ "$method" = lvm ]; then
					echo $path
				fi
			done
			close_dialog
		done
	fi
}
	

# get all unused available physical volumes
# 	in this case all partitions with 0x8e,
#	or all other non-lvm devices from /proc/partitions
#
get_pvs() {
	PARTITIONS=""
	for i in $(enum_pvs); do
		# skip already assigned
		found=no
		for pv in $(vgdisplay -v | grep "[ ]*PV Name" | sed -e "s/ \+PV Name \+//"); do
			[ "$(realpath "$pv")" = "$(realpath "$i")" ] && found=yes
		done
		[ "$found" = "yes" ] && continue

		addinfos_pv "$i"
		i=`printf "%-15s (%s)" "$i" "$RET"`
		
		if [ -z "$PARTITIONS" ]; then
			PARTITIONS="$i"
		else
			PARTITIONS="${PARTITIONS},$i"
		fi
	done
}

#
# list all activated volume groups
#
list_vgs() {
	echo `vgdisplay | grep '^[ ]*VG Name' | sed -e 's/.*[[:space:]]\+\(.*\)$/\1/' | sort`
}

#
# check if any VGs have free space
#
any_free_vgs() {
	for i in $(list_vgs); do
		[ "$(getfree_vg "$i")" = "0" ] || return 0
	done
	return 1
}

#
# get all activated volume groups
#
get_vgs() {
	arg="$1"
	GROUPS=""
	for i in $(list_vgs); do
		if [ "$arg" = "free" -a "$(getfree_vg "$i")" = "0" ]; then
			continue
		fi
		if [ "$arg" = "with-lv" ]; then
			get_vglvs "$i"
			[ "$LVS" = "" ] && continue
		fi
		addinfos_vg "$i"
		i=`printf "%-15s (%s)" "$i" "$RET"`
		
		if [ -z "$GROUPS" ]; then
			GROUPS="$i"
		else
			GROUPS="$GROUPS, $i"
		fi
	done
}

#
# get all physical volumes from a volume group
#
get_vgpvs() {
	PARTITIONS=""
	for i in `vgdisplay -v $1 | grep '^[ ]*PV Name' | 
		sed -e 's,.*/dev,/dev,' | cut -d " " -f 1 | sort`; do

		addinfos_pv "$i"
		i=`printf "%-15s (%s)" "$i" "$RET"`
		
		if [ -z "$PARTITIONS" ]; then
			PARTITIONS="$i"
		else
			PARTITIONS="$PARTITIONS, $i"
		fi
	done
}

#
# get all logical volumes from a volume group
#
get_vglvs() {
	local i
	LVS=""
	for i in `vgdisplay -v $1 | grep '^[ ]*LV Name' | 
		sed -e 's,.*/\(.*\),\1,' | sort`; do

		addinfos_lv "/dev/$1/$i"
		i=`printf "%-15s (%s)" "$i" "$RET"`
		
		if [ -z "$LVS" ]; then
			LVS="$i"
		else
			LVS="$LVS, $i"
		fi
	done
}

#
# this is the VG main-menu
#
vg_mainmenu() {
	while [ 1 ]; do
		db_set lvmcfg/vgmenu "false"
		db_input critical lvmcfg/vgmenu
		db_go
		db_get lvmcfg/vgmenu

		VGRET=`echo "$RET" | \
			sed -e 's/[[:space:]].*//' | \
			tr '[A-Z]' '[a-z']`
		log-output -t lvmcfg vgscan
		case "$VGRET" in
		"create")
			vg_create
			;;
		"delete")
			vg_delete
			;;
		"extend")
			vg_extend
			;;
		"reduce")
			vg_reduce
			;;
		*)
			break
			;;
		esac

		# If we have only created VGs so far, then check if any PVs
		# are still free; if not, leave the menu and default to
		# creating a LV.
		if [ "$FIRST" = "yes" ]; then
			if [ "$VGRET" = "create" ]; then
				get_pvs
				if [ -z "$PARTITIONS" ]; then
					OVERRIDE=no
					db_set lvmcfg/mainmenu "Modify logical volumes (LV)"
					break
				fi
			else
				FIRST=no
			fi
		fi
	done
}

#
# create a new volume group
#
vg_create() {
	# searching for available partitions (0x8e)
	get_pvs
	if [ -z "$PARTITIONS" ]; then
		db_set lvmcfg/nopartitions "false"
		db_input high lvmcfg/nopartitions
		db_go
		return
	fi

	db_subst lvmcfg/vgcreate_parts PARTITIONS $PARTITIONS
	db_set lvmcfg/vgcreate_parts "false"
	db_input critical lvmcfg/vgcreate_parts
	db_go
	db_get lvmcfg/vgcreate_parts
	[ "$RET" = "false" ] && return

	convert_return "$RET"
	PARTITIONS="$RET"

	if [ -z "$RET" ]; then
		db_set lvmcfg/vgcreate_nosel "false"
		db_input high lvmcfg/vgcreate_nosel
		db_go
		return
	fi

	db_set lvmcfg/vgcreate_name ""
	db_input critical lvmcfg/vgcreate_name
	db_go
	[ $? -eq 30 ] && return
	db_get lvmcfg/vgcreate_name
	NAME="$RET"

	# check whether a name for the vg has been given
	if [ "$NAME" = "" ]; then
		db_set lvmcfg/vgcreate_nonamegiven "false"
		db_input high lvmcfg/vgcreate_nonamegiven
		db_go
		return
	fi

	# check, if the vg name is already in use
	vgdisplay -v | grep '^[ ]*VG Name' | sed -e 's/ *VG Name *//' | grep -q "^$NAME$"
	if [ $? -eq 0 ]; then
		db_set lvmcfg/vgcreate_nameused "false"
		db_input high lvmcfg/vgcreate_nameused
		db_go
		return
	fi

	# check if the vg name overlaps with an existing device
	# FIXME: d-i uses devfs while Debian doesn't, so ideally we
	# would also check against a regular /dev tree.
	if [ -e "/dev/$NAME" ]; then
		db_set lvmcfg/vgcreate_devnameused "false"
		db_input high lvmcfg/vgcreate_devnameused
		db_go
		return
	fi

	for p in `echo "$PARTITIONS" | sed -e 's/,/ /g'`; do
		log-output -t lvmcfg pvcreate -ff -y $p
	done

	log-output -t lvmcfg vgcreate "$NAME" `echo "$PARTITIONS" | sed -e 's/,/ /g'`
}

#
# delete a volume group
#
vg_delete() {
	get_vgs
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/vgdelete_novg "false"
		db_input high lvmcfg/vgdelete_novg
		db_go
		return
	fi

	db_subst lvmcfg/vgdelete_names GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/vgdelete_names "false"
	db_input critical lvmcfg/vgdelete_names
	db_go
	db_get lvmcfg/vgdelete_names
	[ "$RET" = "Leave" -o "$RET" = "false" ] && return
	VG=`echo "$RET" | cut -d " " -f1`

	# confirm message
	db_subst lvmcfg/vgdelete_confirm VG $VG
	db_set lvmcfg/vgdelete_confirm "false"
	db_input critical lvmcfg/vgdelete_confirm
	db_go
	db_get lvmcfg/vgdelete_confirm
	[ "$RET" != "true" ] && return

	log-output -t lvmcfg vgchange -a n $VG && \
		log-output -t lvmcfg vgremove $VG
	[ $? -eq 0 ] && return

	# reactivate if deleting was failed
	log-output -t lvmcfg vgchange -a y $VG
	
	db_set lvmcfg/vgdelete_error "false"
	db_input high lvmcfg/vgdelete_error
	db_go
}

#
# allow expanding of existing volume groups
#
vg_extend() {
	# searching for available partitions (0x8e)
	get_pvs
	if [ -z "$PARTITIONS" ]; then
		db_set lvmcfg/nopartitions "false"
		db_input high lvmcfg/nopartitions
		db_go
		return
	fi

	get_vgs
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/vgextend_novg "false"
		db_input high lvmcfg/vgextend_novg
		db_go
		return
	fi

	db_subst lvmcfg/vgextend_names GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/vgextend_names "false"
	db_input critical lvmcfg/vgextend_names
	db_go
	db_get lvmcfg/vgextend_names
	[ "$RET" = "Leave" -o "$RET" = "false" ] && return
	VG=`echo "$RET" | cut -d " " -f1`

	db_subst lvmcfg/vgextend_parts PARTITIONS $PARTITIONS
	db_set lvmcfg/vgextend_parts "false"
	db_input critical lvmcfg/vgextend_parts
	db_go
	db_get lvmcfg/vgextend_parts

	convert_return "$RET"
	PARTITIONS="$RET"

	if [ -z "$RET" ]; then
		db_set lvmcfg/vgextend_nosel "false"
		db_input high lvmcfg/vgextend_nosel
		db_go
		return
	fi

	for p in `echo "$PARTITIONS" | sed -e 's/,/ /g'`; do
		log-output -t lvmcfg pvcreate -ff -y $p && \
			log-output -t lvmcfg vgextend $VG $p
		if [ $? -ne 0 ]; then	# on error
			db_subst lvmcfg/vgextend_error PARTITION $p
			db_subst lvmcfg/vgextend_error VG $VG
			db_set lvmcfg/vgextend_error "false"
			db_input high lvmcfg/vgextend_error
			db_go
			return
		fi
	done
}

#
# allow reducing the volume groups
#
vg_reduce() {
	get_vgs
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/vgreduce_novg "false"
		db_input high lvmcfg/vgreduce_novg
		db_go
		return
	fi

	db_subst lvmcfg/vgreduce_names GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/vgreduce_names "false"
	db_input critical lvmcfg/vgreduce_names
	db_go
	db_get lvmcfg/vgreduce_names
	[ "$RET" = "Leave" -o "$RET" = "false" ] && return
	VG=`echo "$RET" | cut -d " " -f1`

	# check, if the vg has more then one pv's
	set -- `vgdisplay $VG | grep '^[ ]*Cur PV'`
	if [ $3 -le 1 ]; then
		db_subst lvmcfg/vgreduce_minpv VG $VG
		db_set lvmcfg/vgreduce_minpv "false"
		db_input high lvmcfg/vgreduce_minpv
		db_go
		return
	fi

	get_vgpvs "$VG"
	db_subst lvmcfg/vgreduce_parts PARTITIONS $PARTITIONS
	db_set lvmcfg/vgreduce_parts "false"
	db_input critical lvmcfg/vgreduce_parts
	db_go
	db_get lvmcfg/vgreduce_parts
	PARTITIONS=`echo "$RET" | cut -d " " -f1`

	log-output -t lvmcfg vgreduce $VG $PARTITIONS
	if [ $? -ne 0 ]; then
		db_subst lvmcfg/vgreduce_error VG $VG
		db_subst lvmcfg/vgreduce_error PARTITION $PARTITIONS
		db_set lvmcfg/vgreduce_error "false"
		db_input high lvmcfg/vgreduce_error
		db_go
		return
	fi
}

#
# this is the LV main-menu
#
lv_mainmenu() {
	FIRST=yes
	while [ 1 ]; do
		db_set lvmcfg/lvmenu "false"
		db_input critical lvmcfg/lvmenu
		db_go
		db_get lvmcfg/lvmenu

		LVRET=`echo "$RET" | \
			sed -e 's/[[:space:]].*//' | \
			tr '[A-Z]' '[a-z']`
		log-output -t lvmcfg vgscan
		case "$LVRET" in
		"create")
			lv_create
			;;
		"delete")
			lv_delete
			;;
		"extend")
			lv_expand
			;;
		"reduce")
			lv_reduce
			;;
		*)
			break
			;;
		esac

		# Leave the menu and default to "Leave" if there is no
		# free space anymore after setting up LVs.
		if [ "$FIRST" = "yes" ]; then
			if [ "$LVRET" = "create" ]; then
				if ! $(any_free_vgs) ; then
					OVERRIDE=no
					db_set lvmcfg/mainmenu "Leave"
					break
				fi
			else
				FIRST=no
			fi
		fi
	done
}

#
# create a new logical volume
#
lv_create() {
	get_vgs
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/lvcreate_novg "false"
		db_input high lvmcfg/lvcreate_novg
		db_go
		return
	fi
	get_vgs "free"
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/lvcreate_nofreevg "false"
		db_input high lvmcfg/lvcreate_nofreevg
		db_go
		return
	fi

	db_set lvmcfg/lvcreate_name ""
	db_input critical lvmcfg/lvcreate_name
	db_go
	[ $? -eq 30 ] && return
	db_get lvmcfg/lvcreate_name
	NAME="$RET"

	# check whether a name for the lv has been given
	if [ "$NAME" = "" ]; then
		db_set lvmcfg/lvcreate_nonamegiven "false"
		db_input high lvmcfg/lvcreate_nonamegiven
		db_go
		return
	fi

	db_subst lvmcfg/lvcreate_vgnames GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/lvcreate_vgnames "false"
	db_input critical lvmcfg/lvcreate_vgnames
	db_go
	db_get lvmcfg/lvcreate_vgnames
	[ "$RET" = "Leave" -o "$RET" = "false" ] && return
	VG=`echo "$RET" | cut -d " " -f1`

	# make sure, the name isn't already in use
	vgdisplay -v $VG | grep '^[ ]*LV Name' | grep -q "${VG}/${NAME}$"
	if [ $? -eq 0 ]; then
		db_subst lvmcfg/lvcreate_exists LV "$NAME"
		db_subst lvmcfg/lvcreate_exists VG $VG
		db_set lvmcfg/lvcreate_exists "false"
		db_input high lvmcfg/lvcreate_exists
		db_go
		return
	fi

	MAX_SIZE=$(human2lvm "$(getfree_vg "$VG")")
	db_set lvmcfg/lvcreate_size "$MAX_SIZE"
	db_fset lvmcfg/lvcreate_size seen false
	db_input critical lvmcfg/lvcreate_size
	db_go
	db_get lvmcfg/lvcreate_size
	SIZE=$(human2lvm "$RET")
	[ -z "$RET" ] && return

	# If the maximum free space should be used for the new LV, use the
	# number of physical extents (PEs) because the size given by LVM
	# might not be accurate, resulting in an error because the VG is
	# not big enough (see #250594).
	if [ "$MAX_SIZE" = "$SIZE" ]; then
		log-output -t lvmcfg lvcreate -l$(getfreepe_vg "$VG") -n "$NAME" $VG
	else
		log-output -t lvmcfg lvcreate -L${SIZE} -n "$NAME" $VG
	fi

	if [ $? -ne 0 ]; then
		db_subst lvmcfg/lvcreate_error VG $VG
		db_subst lvmcfg/lvcreate_error LV $NAME
		db_subst lvmcfg/lvcreate_error SIZE $SIZE
		db_set lvmcfg/lvcreate_error "false"
		db_input high lvmcfg/lvcreate_error
		db_go
		return
	fi
}


#
# create a new logical volume
#
lv_delete() {
	get_vgs "with-lv"
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/lvdelete_novg "false"
		db_input high lvmcfg/lvdelete_novg
		db_go
		return
	fi

	db_subst lvmcfg/lvdelete_vgnames GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/lvdelete_vgnames "false"
	db_input critical lvmcfg/lvdelete_vgnames
	db_go
	db_get lvmcfg/lvdelete_vgnames
	[ "$RET" = "Leave" -o "$RET" = "false" ] && return
	VG=`echo "$RET" | cut -d " " -f1`

	get_vglvs "$VG"
	if [ -z "$LVS" ]; then
		db_set lvmcfg/lvdelete_nolv "false"
		db_input high lvmcfg/lvdelete_nolv
		db_go
		return
	fi

	db_subst lvmcfg/lvdelete_lvnames VG "$VG"
	db_subst lvmcfg/lvdelete_lvnames LVS "${LVS}, Leave"
	db_set lvmcfg/lvdelete_lvnames "false"
	db_input critical lvmcfg/lvdelete_lvnames
	db_go
	db_get lvmcfg/lvdelete_lvnames
	[ "$RET" = "Leave" -o "$RET" = "false" ] && return
	LV=`echo "$RET" | cut -d " " -f1`

	log-output -t lvmcfg lvremove -f /dev/${VG}/${LV}
	if [ $? -ne 0 ]; then
		db_subst lvmcfg/lvdelete_error VG $VG
		db_subst lvmcfg/lvdelete_error LV $LV
		db_set lvmcfg/lvdelete_error "false"
		db_input high lvmcfg/lvdelete_error
		db_go
		return
	fi
}


#
# *** main stuff ***
#

# load required kernel modules
depmod -a 1>/dev/null 2>&1
modprobe dm-mod >/dev/null 2>&1
modprobe lvm-mod >/dev/null 2>&1

# make sure that lvm is available
if ! grep -q "[0-9] device-mapper$" /proc/misc ; then
	db_set lvmcfg/nolvm "false"
	db_input high lvmcfg/nolvm
	db_go
	exit 0
fi

# scan for logical volumes
log-output -t lvmcfg pvscan
log-output -t lvmcfg vgscan

# try to activate old volume groups
set -- `vgdisplay -v | grep 'NOT \(active\|available\)' | wc -l`
if [ $1 -gt 0 -a ! -f /var/cache/lvmcfg/first ]; then
	db_subst lvmcfg/activevg COUNT $1
	db_set lvmcfg/activevg "false"
	db_input critical lvmcfg/activevg
	db_go
	db_get lvmcfg/activevg
	[ "$RET" = "true" ] && log-output -t lvmcfg vgchange -a y
fi
# ask only the first time
mkdir -p /var/cache/lvmcfg && touch /var/cache/lvmcfg/first

FIRST=yes
# The following variable determines whether the lvmcfg/mainmenu is set
# to false.  At various places in this script, OVERRIDE=no is set in
# order to set a default.  This is useful to have LV as default when
# all VGs have been set up, and Leave the default after all LVs have
# been allocated.
OVERRIDE=yes
# main-loop
while [ 1 ]; do
	[ "$OVERRIDE" = "yes" ] && db_set lvmcfg/mainmenu "false"
	db_input critical lvmcfg/mainmenu
	db_go
	db_get lvmcfg/mainmenu
	MAINRET=`echo "$RET" | sed -e 's,.*(\(.*\)).*,\1,'`

	OVERRIDE=yes
	case "$MAINRET" in
	"PV")	# currently unused
		#pv_mainmenu
		;;
	"VG")
		vg_mainmenu
		;;
	"LV")
		lv_mainmenu
		;;
	*)
		break	# finished
		;;
	esac
done

# install lvm tools in /target if needed
set -- `vgdisplay -v | grep -i "Logical volume ---" | wc -l`
[ $1 -gt 0 ] && apt-install lvm2

exit 0
