#!/bin/sh

. /usr/share/debconf/confmodule
. /usr/share/debian-installer/functions.sh

#
# get all unused available physical volumes
# 	in this case all partitions with 0x8e,
#	or all other non-lvm devices from /proc/partitions
#
get_pvs() {
	export FSID="8e" && get_partitions
	PARTITIONS=""
	for i in `echo "$RET" | sed -e 's/,/ /g'`; do
		# skip already assigned
		vgdisplay -v | grep -q "$i"
		[ $? -eq 0 ] && continue

		if [ -z "$PARTITIONS" ]; then
			PARTITIONS="$i"
		else
			PARTITIONS="${PARTITIONS},$i"
		fi
	done
}

#
# get all activated volume groups
#
get_vgs() {
	GROUPS=""
	for i in `vgdisplay | grep '^VG Name' | 
		sed -e 's/.*[[:space:]]\(.*\)$/\1/' | sort`; do
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
	for i in `vgdisplay -v $1 | grep '^PV Name' | 
		sed -e 's,.*/dev\(.*\)(.*,/dev\1,' | sort`; do
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
	LVS=""
	for i in `vgdisplay -v $1 | grep '^LV Name' | 
		sed -e 's,.*/\(.*\),\1,' | sort`; do
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
		db_input high lvmcfg/vgmenu
		db_go
		db_get lvmcfg/vgmenu

		VGRET=`echo "$RET" | \
			sed -e 's,^\(.*\)[[:space:]].*,\1,' | \
			tr '[A-Z]' '[a-z']`
		vgscan >>/var/log/messages 2>&1
		case "$VGRET" in
		"create")
			vg_create
			;;
		"delete")
			vg_delete
			;;
		"extend")
			vg_expand
			;;
		"reduce")
			vg_reduce
			;;
		*)
			break
			;;
		esac
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
	db_input high lvmcfg/vgcreate_parts
	db_go
	db_get lvmcfg/vgcreate_parts
	PARTITIONS="$RET"

	if [ -z "$RET" ]; then
		db_set lvmcfg/vgcreate_nosel "false"
		db_input high lvmcfg/vgcreate_nosel
		db_go
		return
	fi

	db_set lvmcfg/vgcreate_name ""
	db_input high lvmcfg/vgcreate_name
	db_go
	db_get lvmcfg/vgcreate_name
	NAME="$RET"

	# check, if the vg name is already in use
	vgdisplay -v | grep '^VG Name' | grep -q "$NAME"
	if [ $? -eq 0 ]; then
		db_set lvmcfg/vgcreate_nameused "false"
		db_input high lvmcfg/vgcreate_nameused
		db_go
		return
	fi

	for p in `echo "$PARTITIONS" | sed -e 's/,/ /g'`; do
		pvcreate -ff -y $p >>/var/log/messages 2>&1
	done

	vgcreate "$NAME" `echo "$PARTITIONS" | sed -e 's/,/ /g'` \
		>>/var/log/messages 2>&1
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
	db_input high lvmcfg/vgdelete_names
	db_go
	db_get lvmcfg/vgdelete_names
	[ "$RET" = "Leave" ] && return
	VG="$RET"

	# confirm message
	db_subst lvmcfg/vgdelete_confirm VG $VG
	db_set lvmcfg/vgdelete_confirm "false"
	db_input high lvmcfg/vgdelete_confirm
	db_go
	db_get lvmcfg/vgdelete_confirm
	[ "$RET" != "true" ] && return

	vgchange -a n $VG >>/var/log/messages 2>&1 && \
		vgremove $VG >>/var/log/messages 2>&1
	[ $? -eq 0 ] && return

	# reactivate if deleting was failed
	vgchange -a y $VG >>/var/log/messages 2>&1
	
	db_set lvmcfg/vgdelete_error "false"
	db_input high lvmcfg/vgdelete_error
	db_go
}

#
# allow expanding of existing volume groups
#
vg_expand() {
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
	db_input high lvmcfg/vgextend_names
	db_go
	db_get lvmcfg/vgextend_names
	[ "$RET" = "Leave" ] && return
	GROUP="$RET"

	db_subst lvmcfg/vgextend_parts PARTITIONS $PARTITIONS
	db_set lvmcfg/vgextend_parts "false"
	db_input high lvmcfg/vgextend_parts
	db_go
	db_get lvmcfg/vgextend_parts
	PARTITIONS="$RET"
	if [ -z "$RET" ]; then
		db_set lvmcfg/vgextend_nosel "false"
		db_input high lvmcfg/vgextend_nosel
		db_go
		return
	fi

	for p in `echo "$PARTITIONS" | sed -e 's/,/ /g'`; do
		pvcreate -ff -y $p >>/var/log/messages 2>&1 && \
			vgextend $GROUP $p >>/var/log/messages 2>&1
		if [ $? -ne 0 ]; then	# on error
			db_subst lvmcfg/vgextend_error PARTITION $p
			db_subst lvmcfg/vgextend_error VG $GROUP
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
	db_input high lvmcfg/vgreduce_names
	db_go
	db_get lvmcfg/vgreduce_names
	[ "$RET" = "Leave" ] && return
	VG="$RET"

	# check, if the vg has more then one pv's
	set -- `vgdisplay $VG | grep '^Cur PV'`
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
	db_input high lvmcfg/vgreduce_parts
	db_go
	db_get lvmcfg/vgreduce_parts
	PARTITIONS="$RET"

	vgreduce $VG $PARTITIONS >>/var/log/messages 2>&1
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
	while [ 1 ]; do
		db_set lvmcfg/lvmenu "false"
		db_input high lvmcfg/lvmenu
		db_go
		db_get lvmcfg/lvmenu

		LVRET=`echo "$RET" | \
			sed -e 's,^\(.*\)[[:space:]].*,\1,' | \
			tr '[A-Z]' '[a-z']`
		vgscan >>/var/log/messages 2>&1
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

	db_set lvmcfg/lvcreate_name ""
	db_input high lvmcfg/lvcreate_name
	db_go
	db_get lvmcfg/lvcreate_name
	NAME="$RET"

	db_subst lvmcfg/lvcreate_vgnames GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/lvcreate_vgnames "false"
	db_input high lvmcfg/lvcreate_vgnames
	db_go
	db_get lvmcfg/lvcreate_vgnames
	[ "$RET" = "Leave" ] && return
	VG="$RET"

	# make sure, the name isn't already in use
	vgdisplay -v $VG | grep '^LV Name' | grep -q "${VG}/${NAME}$"
	if [ $? -eq 0 ]; then
		db_subst lvmcfg/lvcreate_exists LV "$NAME"
		db_subst lvmcfg/lvcreate_exists VG $VG
		db_set lvmcfg/lvcreate_exists "false"
		db_input high lvmcfg/lvcreate_exists
		db_go
		return
	fi

	db_input high lvmcfg/lvcreate_size
	db_go
	db_get lvmcfg/lvcreate_size
	SIZE="$RET"
	[ -z "$RET" ] && return

	lvcreate -L${SIZE} -n "$NAME" $VG >>/var/log/messages 2>&1
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
	get_vgs
	if [ -z "$GROUPS" ]; then
		db_set lvmcfg/lvdelete_novg "false"
		db_input high lvmcfg/lvdelete_novg
		db_go
		return
	fi

	db_subst lvmcfg/lvdelete_vgnames GROUPS "${GROUPS}, Leave"
	db_set lvmcfg/lvdelete_vgnames "false"
	db_input high lvmcfg/lvdelete_vgnames
	db_go
	db_get lvmcfg/lvdelete_vgnames
	[ "$RET" = "Leave" ] && return
	VG="$RET"

	get_vglvs "$VG"
	if [ -z "$LVS" ]; then
		db_set lvmcfg/lvdelete_nolv "false"
		db_input high lvmcfg/lvdelete_nolv
		db_go
		return
	fi

	db_subst lvmcfg/lvdelete_lvnames LVS "${LVS}, Leave"
	db_set lvmcfg/lvdelete_lvnames "false"
	db_input high lvmcfg/lvdelete_lvnames
	db_go
	db_get lvmcfg/lvdelete_lvnames
	[ "$RET" = "Leave" ] && return
	LV="$RET"

	lvremove -f /dev/${VG}/${LV} >>/var/log/messages 2>&1
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
modprobe lvm-mod >/dev/null 2>&1

# scan for logical volumes
pvscan >>/var/log/messages 2>&1
vgscan >>/var/log/messages 2>&1

# try to activate old volume groups
set -- `vgdisplay -v | grep 'NOT active' | wc -l`
if [ $1 -gt 0 -a ! -f /var/cache/lvmcfg/first ]; then
	db_subst lvmcfg/activevg COUNT $1
	db_set lvmcfg/activevg "false"
	db_input high lvmcfg/activevg
	db_go
	db_get lvmcfg/activevg
	[ "$RET" = "true" ] && vgchange -a y >>/var/log/messages 2>&1
fi
# ask only the first time
mkdir -p /var/cache/lvmcfg && touch /var/cache/lvmcfg/first

# main-loop
while [ 1 ]; do
	db_set lvmcfg/mainmenu "false"
	db_input high lvmcfg/mainmenu
	db_go
	db_get lvmcfg/mainmenu
	MAINRET=`echo "$RET" | sed -e 's,.*(\(.*\)).*,\1,'`

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

exit 0

