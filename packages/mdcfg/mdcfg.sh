#!/bin/sh

### Magic debconf stuff :-) ###
. /usr/share/debconf/confmodule

### Functions ###
md_get_devices() {
	DEVICES=""
	for i in `grep ^md /proc/mdstat|sed -e 's/^\(md.*\) : active \([[:alnum:]]*\).*/\1_\2/'`; do
		if [ -z "$DEVICES" ]; then
			DEVICES="${i}"
		else
			DEVICES="${DEVICES}, ${i}"
		fi
	done
}

md_delete_verify() {
	DEVICE=`echo "$1" | sed -e "s/^\(md.*\)_.*/\1/"`
	INFO=`grep "^${DEVICE}[ :]" /proc/mdstat | sed -e "s/^md.* : active \([[:alnum:]]*\) \(.*\)/\1:\2/"`
	TYPE=`echo ${INFO}|sed -e "s/\(.*\):.*/\1/"`
	DEVICES=`echo ${INFO}|sed -e "s/.*:\(.*\)/\1/"`
	NUMBER=`echo ${DEVICE}|sed -e "s/^md//"`
	db_set mdcfg/deleteverify "false"
	db_subst mdcfg/deleteverify DEVICE "/dev/${DEVICE}"
	db_subst mdcfg/deleteverify TYPE "${TYPE}"
	db_subst mdcfg/deleteverify DEVICES "${DEVICES}"
	db_input critical mdcfg/deleteverify
	db_go
	db_get mdcfg/deleteverify

	case $RET in 
		"true")
			# Stop the MD device, and zero the superblock
			# of all the component devices
      DEVICES=`mdadm -Q --detail /dev/md/${NUMBER} | grep "\(active\|spare\)" | sed -e 's/.* //'`
			mdadm --stop /dev/md/${NUMBER}
			echo "Removing /dev/md/$NUMBER" > /var/log/mdcfg.log
			echo "${DEVICES}" >> /var/log/mdcfg.log
			for DEV in "$DEVICES"; do
				mdadm --zero-superblock --force ${DEV}
				echo ${DEV} >> /var/log/mdcfg.log
			done
			;;
		"false") ;;
	esac
}

md_delete() {
	md_get_devices
	if [ -z "$DEVICES" ]; then
		db_set mdcfg/delete_no_md "false"
		db_input high mdcfg/delete_no_md
		db_go
		return
	fi
	db_set mdcfg/deletemenu "false"
	db_subst mdcfg/deletemenu DEVICES "${DEVICES}"
	db_input critical mdcfg/deletemenu
	db_go
	db_get mdcfg/deletemenu

	case $RET in
		md*)
			md_delete_verify $RET;;
		*);;
	esac
}

md_createmain() {
	db_set mdcfg/createmain "false"
	db_input critical mdcfg/createmain
	db_go
	if [ $? -ne "30" ]; then
		db_get mdcfg/createmain
		
		get_partitions

		case $RET in
			"RAID5")
			md_create_raid5;;
			"RAID1")
			md_create_raid1;;
			"RAID0")
			md_create_raid0;;
		esac
	fi
}

# this will set PARTITIONS and NUM_PART global variables.
get_partitions() {
	PARTITIONS=""

	# Get a list of RAID partitions. This only works if there is no 
	# filesystem on the partitions, which is fine by us.
	RAW_PARTITIONS=`/usr/lib/partconf/find-partitions --ignore-fstype 2>/dev/null | grep "[[:space:]]RAID[[:space:]]" |cut -f1`;

	# Convert it into a proper list form for a select question 
	# (comma seperated)
	NUM_PART=0
	for i in ${RAW_PARTITIONS}; do
		DEV=`echo ${i}|sed -e "s/\/dev\///"`
		MAPPEDDEV=`mapdevfs ${i} | sed -e "s/\/dev\///"`

		if grep -q "\(${DEV}\|${MAPPEDDEV}\)" /proc/mdstat; then
			continue
		fi

		if [ -z "${PARTITIONS}" ] ; then
			PARTITIONS="${i}"
		else
			PARTITIONS="${PARTITIONS}, ${i}"
		fi
		NUM_PART=$(($NUM_PART + 1))
	done

	if [ -z "${PARTITIONS}" ] ; then
		db_input critical mdcfg/noparts
		db_go
		return
	fi
}

prune_partitions() {
	CHOSEN="$1"
	OLDIFS="$IFS"
	IFS=,
	NEW_PARTITIONS=""
	for i in $PARTITIONS; do
		found=0
		for j in $CHOSEN; do
			if [ "$i" = "$j" ]; then
				found=1
			fi
		done
		if [ $found -eq 0 ]; then
			if [ -z "$NEW_PARTITIONS" ]; then
				NEW_PARTITIONS="$i"
			else
				NEW_PARTITIONS="${NEW_PARTITIONS},$i"
			fi
		fi
	done
	IFS=$OLDIFS
	PARTITIONS=$NEW_PARTITIONS
}

md_create_raid0() {
	db_subst mdcfg/raid0devs PARTITIONS "${PARTITIONS}"
	db_set mdcfg/raid0devs ""
	db_input critical mdcfg/raid0devs
	db_go

	if [ "$?" -eq 30 ]; then return; fi

	db_get mdcfg/raid0devs
	SELECTED=0
	for i in $RET; do
		let SELECTED++
	done

	prune_partitions "$RET"

	MD_NUM=`grep ^md /proc/mdstat|sed -e 's/^md\(.*\) : active .*/\1/'|sort|tail -n 1`

	if [ -z "${MD_NUM}" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	echo "Number of devices in the RAID0 array md${MD_NUM}: ${SELECTED}"

	RAID_DEVICES="$(echo ${RET} | sed -e 's/,//g')"
	echo "Commandline:"
	`mdadm --create /dev/md/${MD_NUM} --auto=yes --force -R -l raid0 -n ${SELECTED} ${RAID_DEVICES}`
}

md_create_raid1() {
	OK=0

	db_set mdcfg/raid1devcount "2"

	# Get the count of active devices
	while [ "${OK}" -eq 0 ]; do
		db_input critical mdcfg/raid1devcount
		db_go
		if [ "$?" -eq "30" ]; then
			# If the user has pressed "Cancel", return
			return
		fi

		# Figure out, if the user entered a number
		db_get mdcfg/raid1devcount
		RET=`echo ${RET}|sed -e "s/[[:space:]]//g"`
		if [ ! -z "${RET}" ]; then
			let "OK=${RET}>0 && ${RET}<99"
		fi
	done

	db_set mdcfg/raid1sparecount "0"
	OK=0

	# Same procedure as above, but get the number of spare partitions
	# this time.
	# TODO: Make a general function for this kind of stuff
	while [ "${OK}" -eq 0 ]; do
		db_input critical mdcfg/raid1sparecount
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi
		db_get mdcfg/raid1sparecount
		RET=`echo ${RET}|sed -e "s/[[:space:]]//g"`
		if [ ! -z "${RET}" ]; then
			let "OK=${RET}>=0 && ${RET}<99"
		fi
	done

	db_get mdcfg/raid1devcount
	DEV_COUNT="${RET}"
	db_get mdcfg/raid1sparecount
	SPARE_COUNT="${RET}"
	REQUIRED=$(($DEV_COUNT + $SPARE_COUNT))

	db_set mdcfg/raid1devs ""
	SELECTED=0

	# Loop until at least one device has been selected
	until [ "${SELECTED}" -gt "0" -a "${SELECTED}" -le "${DEV_COUNT}" ]; do
		db_subst mdcfg/raid1devs COUNT "${DEV_COUNT}"
		db_subst mdcfg/raid1devs PARTITIONS "${PARTITIONS}"
		db_input critical mdcfg/raid1devs
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi

		db_get mdcfg/raid1devs
		SELECTED=0
		for i in $RET; do
			DEVICE=`echo ${i}|sed -e "s/,//"`
			let SELECTED++
		done
	done

	# Add "missing" for as many devices as weren't selected
	while [ "${SELECTED}" -lt "${DEV_COUNT}" ]; do
		MISSING_DEVICES="${MISSING_DEVICES} missing"
		let SELECTED++
	done

	# Remove partitions selected in raid1devs from the PARTITION list
	db_get mdcfg/raid1devs

	prune_partitions "$RET"

	db_set mdcfg/raid1sparedevs ""
	SELECTED=0
	if [ "${SPARE_COUNT}" -gt 0 ]; then  
		FIRST=1
		# Loop until the correct amount of devices has been selected.
	  # That means any number less than or equal to the spare count
		while [ "${SELECTED}" -gt "${SPARE_COUNT}" -o "${FIRST}" -eq 1 ]; do
			FIRST=0
			db_subst mdcfg/raid1sparedevs COUNT "${SPARE_COUNT}"
			db_subst mdcfg/raid1sparedevs PARTITIONS "${PARTITIONS}"
			db_input critical mdcfg/raid1sparedevs
			db_go
			if [ "$?" -eq "30" ]; then
				return
			fi
	
			db_get mdcfg/raid1sparedevs
			SELECTED=0
			for i in $RET; do
				DEVICE=`echo ${i}|sed -e "s/,//"`
				let SELECTED++
			done
		done
	fi

	# The amount of spares, the user has selected
	NAMED_SPARES=${SELECTED}

	db_get mdcfg/raid1devs
	RAID_DEVICES=`echo ${RET}|sed -e "s/,//g"`

	db_get mdcfg/raid1sparedevs
	SPARE_DEVICES=`echo ${RET}|sed -e "s/,//g"`

	MISSING_SPARES=""

	COUNT=${NAMED_SPARES}
	while [ "${COUNT}" -lt "${SPARE_COUNT}" ]; do
		MISSING_SPARES="${MISSING_SPARES} missing"
		let COUNT++
	done

	# Find the next available md-number
	MD_NUM=`grep ^md /proc/mdstat|sed -e 's/^md\(.*\) : active .*/\1/'|sort|tail -n 1`
	if [ -z "${MD_NUM}" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	echo "Selected spare count: ${NAMED_SPARES}"
	echo "Raid devices count: ${DEV_COUNT}"
	echo "Spare devices count: ${SPARE_COUNT}"
	echo "Commandline:"
	`mdadm --create /dev/md/${MD_NUM} --auto=yes --force -R -l raid1 -n ${DEV_COUNT} -x ${SPARE_COUNT} ${RAID_DEVICES} ${MISSING_DEVICES} ${SPARE_DEVICES} ${MISSING_SPARES}`
}

md_create_raid5() {
	OK=0

	db_set mdcfg/raid5devcount "3"

	# Get the count of active devices
	while [ "${OK}" -eq 0 ]; do
		db_input critical mdcfg/raid5devcount
		db_go
		if [ "$?" -eq "30" ]; then
			# If the user has pressed "Cancel", return
			return
		fi

		# Figure out, if the user entered a number
		db_get mdcfg/raid5devcount
		RET=`echo ${RET}|sed -e "s/[[:space:]]//g"`
		if [ ! -z "${RET}" ]; then
			let "OK=${RET}>0 && ${RET}<99"
		fi
	done

	db_set mdcfg/raid5sparecount "0"
	OK=0

	# Same procedure as above, but get the number of spare partitions
	# this time.
	# TODO: Make a general function for this kind of stuff
	while [ "${OK}" -eq 0 ]; do
		db_input critical mdcfg/raid5sparecount
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi
		db_get mdcfg/raid5sparecount
		RET=`echo ${RET}|sed -e "s/[[:space:]]//g"`
		if [ ! -z "${RET}" ]; then
			let "OK=${RET}>=0 && ${RET}<99"
		fi
	done

	db_get mdcfg/raid5devcount
	DEV_COUNT="${RET}"
	if [ "$DEV_COUNT" -lt 3 ] ; then
		DEV_COUNT=3 # Minimum number for RAID5
	fi
	db_get mdcfg/raid5sparecount
	SPARE_COUNT="${RET}"
	REQUIRED=$(($DEV_COUNT + $SPARE_COUNT))
	if [ "$REQUIRED" -gt "$NUM_PART" ] ; then
		db_subst mdcfg/notenoughparts NUM_PART "${NUM_PART}"
		db_subst mdcfg/notenoughparts REQUIRED "${REQUIRED}"
		db_input critical mdcfg/notenoughparts
		db_go mdcfg/notenoughparts
		return
	fi

	db_set mdcfg/raid5devs ""
	SELECTED=0

	# Loop until the correct amount of active devices has been selected
	while [ "${SELECTED}" -ne "${DEV_COUNT}" ]; do
		db_subst mdcfg/raid5devs COUNT "${DEV_COUNT}"
		db_subst mdcfg/raid5devs PARTITIONS "${PARTITIONS}"
		db_input critical mdcfg/raid5devs
		db_go
		if [ "$?" -eq "30" ]; then
			return
		fi

		db_get mdcfg/raid5devs
		SELECTED=0
		for i in $RET; do
			DEVICE=`echo ${i}|sed -e "s/,//"`
			let SELECTED++
		done
	done

	# Remove partitions selected in raid5devs from the PARTITION list
	db_get mdcfg/raid5devs

	prune_partitions "$RET"

	db_set mdcfg/raid5sparedevs ""
	SELECTED=0
	if [ "${SPARE_COUNT}" -gt 0 ]; then  
		FIRST=1
		# Loop until the correct amount of devices has been selected.
	  # That means any number less than or equal to the spare count
		while [ "${SELECTED}" -gt "${SPARE_COUNT}" -o "${FIRST}" -eq 1 ]; do
			FIRST=0
			db_subst mdcfg/raid5sparedevs COUNT "${SPARE_COUNT}"
			db_subst mdcfg/raid5sparedevs PARTITIONS "${PARTITIONS}"
			db_input critical mdcfg/raid5sparedevs
			db_go
			if [ "$?" -eq "30" ]; then
				return
			fi
	
			db_get mdcfg/raid5sparedevs
			SELECTED=0
			for i in $RET; do
				DEVICE=`echo ${i}|sed -e "s/,//"`
				let SELECTED++
			done
		done
	fi

	# The amount of spares, the user has selected
	NAMED_SPARES=${SELECTED}

	db_get mdcfg/raid5devs
	RAID_DEVICES=`echo ${RET}|sed -e "s/,//g"`

	db_get mdcfg/raid5sparedevs
	SPARE_DEVICES=`echo ${RET}|sed -e "s/,//g"`

	MISSING_SPARES=""

	COUNT=${NAMED_SPARES}
	while [ "${COUNT}" -lt "${SPARE_COUNT}" ]; do
		MISSING_SPARES="${MISSING_SPARES} missing"
		let COUNT++
	done

	# Find the next available md-number
	MD_NUM=`grep ^md /proc/mdstat|sed -e 's/^md\(.*\) : active .*/\1/'|sort|tail -n 1`
	if [ -z "${MD_NUM}" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	echo "Selected spare count: ${NAMED_SPARES}"
	echo "Raid devices count: ${DEV_COUNT}"
	echo "Spare devices count: ${SPARE_COUNT}"
	echo "Commandline:"
	`mdadm --create /dev/md/${MD_NUM} --auto=yes --force -R -l raid5 -n ${DEV_COUNT} -x ${SPARE_COUNT} ${RAID_DEVICES} ${SPARE_DEVICES} ${MISSING_SPARES}`
}

md_mainmenu() {
	while [ 1 ]; do
		db_set mdcfg/mainmenu "false"
		db_input critical mdcfg/mainmenu
		db_go
		if [ $? -eq "30" ]; then
			exit 30
		fi
		db_get mdcfg/mainmenu
		case $RET in
			"Create MD device")
				md_createmain;;
			"Delete MD device")
				md_delete;;
			"Finish")
				break;;
		esac
	done
}

### Main of script ###

# Try to load the necesarry modules.
# Supported schemes: RAID 0, RAID 1, RAID 5
depmod -a 1>/dev/null 2>&1
modprobe md 1>/dev/null 2>&1 || modprobe md-mod 1>/dev/null 2>&1
modprobe raid0 >/dev/null 2>&1
modprobe raid1 1>/dev/null 2>&1
modprobe raid5 >/dev/null 2>&1
mkdir -p /dev/md

# Try to detect MD devices, and start them
/sbin/mdrun

# Make sure that we have md-support
if [ ! -e /proc/mdstat ]; then
	db_set mdcfg/nomd "false"
	db_input high mdcfg/nomd
	db_go
	db_stop
	exit 0
fi

# Force mdadm to be installed on the target system
apt-install mdadm

# We want the "go back" button
#db_capb backup

md_mainmenu

#db_stop
exit 0
