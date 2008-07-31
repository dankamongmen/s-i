#!/bin/sh

. /usr/share/debconf/confmodule

# Translate device from /proc/mdstat (mdX) to device node
md_devnode() {
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

md_get_level() {
	echo $(mdadm -Q --detail $1 | grep "Raid Level" | sed "s/.*: //")
}

md_get_devices() {
	DEVICES=""
	for DEVICE in $(grep ^md /proc/mdstat | \
		   sed -e 's/^\(md.*\) : .*/\1/'); do
		MDDEV=$(md_devnode $DEVICE) || return 1
		TYPE=$(md_get_level $MDDEV)
		DEVICES="${DEVICES:+$DEVICES, }${DEVICE}_$TYPE"
	done
}

md_delete_verify() {
	DEVICE=$(echo "$1" | sed -e "s/^\(md.*\)_.*/\1/")
	DEVICES=$(grep "^$DEVICE[ :]" /proc/mdstat | \
		sed -e "s/^.*active \(.*\)/\1/; s/raid[0-9]* //")
	MDDEV=$(md_devnode $DEVICE) || return 1
	TYPE=$(md_get_level $MDDEV)

	db_set mdcfg/deleteverify false
	db_subst mdcfg/deleteverify DEVICE "/dev/$DEVICE"
	db_subst mdcfg/deleteverify TYPE "$TYPE"
	db_subst mdcfg/deleteverify DEVICES "$DEVICES"
	db_input critical mdcfg/deleteverify
	db_go
	db_get mdcfg/deleteverify

	case $RET in
	    true)
		# Stop the MD device and zero the superblock
		# of all the component devices
		DEVICES=$(mdadm -Q --detail $MDDEV | \
			  grep -E "^[[:space:]]*[0-9].*(active|spare)" | \
			  sed -e 's/.* //')
		logger -t mdcfg "Removing $MDDEV ($DEVICES)"
		log-output -t mdcfg mdadm --stop $MDDEV || return 1
		for DEV in "$DEVICES"; do
			log-output -t mdcfg \
				mdadm --zero-superblock --force $DEV || return 1
		done
		;;
	esac
	return 0
}

md_delete() {
	md_get_devices
	if [ -z "$DEVICES" ]; then
		db_set mdcfg/delete_no_md false
		db_input high mdcfg/delete_no_md
		db_go
		return
	fi
	db_set mdcfg/deletemenu false
	db_subst mdcfg/deletemenu DEVICES "$DEVICES"
	db_input critical mdcfg/deletemenu
	db_go
	db_get mdcfg/deletemenu

	case $RET in
	    md*)
		if ! md_delete_verify $RET; then
			db_input critical mdcfg/deletefailed
			db_go
		fi
		;;
	esac
}

md_createmain() {
	db_set mdcfg/createmain false
	db_input critical mdcfg/createmain
	db_go
	if [ $? -ne 30 ]; then
		db_get mdcfg/createmain
		if [ "$RET" = Cancel ]; then
			return
		fi
		RAID_SEL="$RET"

		if ! get_partitions; then
			return
		fi

		case "$RAID_SEL" in
		    RAID6|RAID5|RAID1)
			md_create_array "$RAID_SEL" ;;
		    RAID0)
			md_create_raid0 ;;
		    *)
			return 1 ;;
		esac
	fi
}

# This will set PARTITIONS and NUM_PART global variables
get_partitions() {
	PARTITIONS=""

	# Get a list of RAID partitions. This only works if there is no
	# filesystem on the partitions, which is fine by us.
	RAW_PARTITIONS=$(/usr/lib/partconf/find-partitions --ignore-fstype 2>/dev/null | \
		grep "[[:space:]]RAID[[:space:]]" | cut -f1)

	# Convert it into a proper list form for a select question
	# (comma seperated)
	NUM_PART=0
	for i in $RAW_PARTITIONS; do
		DEV=$(echo $i | sed -e "s/\/dev\///")
		REALDEV=$(mapdevfs "$i")
		MAPPEDDEV=$(echo "$REALDEV" | sed -e "s/\/dev\///")

		if grep -Eq "($DEV|$MAPPEDDEV)" /proc/mdstat; then
			continue
		fi

		PARTITIONS="${PARTITIONS:+$PARTITIONS, }$REALDEV"
		NUM_PART=$(($NUM_PART + 1))
	done

	if [ -z "$PARTITIONS" ]; then
		db_input critical mdcfg/noparts
		db_go
		return 1
	fi
	return 0
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
			NEW_PARTITIONS="${NEW_PARTITIONS:+$NEW_PARTITIONS,}$i"
		fi
	done
	IFS=$OLDIFS
	PARTITIONS=$NEW_PARTITIONS
}

md_create_raid0() {
	db_subst mdcfg/raid0devs PARTITIONS "$PARTITIONS"
	db_set mdcfg/raid0devs ""
	db_input critical mdcfg/raid0devs
	db_go

	if [ $? -eq 30 ]; then return; fi

	db_get mdcfg/raid0devs
	SELECTED=0
	for i in $RET; do
		let SELECTED++
	done

	prune_partitions "$RET"

	MD_NUM=$(grep ^md /proc/mdstat | \
		 sed -e 's/^md\(.*\) : active .*/\1/' | sort | tail -n1)

	if [ -z "$MD_NUM" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	logger -t mdcfg "Number of devices in the RAID0 array md$MD_NUM: $SELECTED"

	RAID_DEVICES="$(echo $RET | sed -e 's/,//g')"
	log-output -t mdcfg \
		mdadm --create /dev/md$MD_NUM --auto=yes --force -R -l raid0 \
		      -n $SELECTED $RAID_DEVICES
}

md_create_array(){
	OK=0

	case "$1" in
	    RAID1)
		MIN_SIZE=2 ;;
	    RAID5)
		MIN_SIZE=3 ;;
	    RAID6)
		MIN_SIZE=4 ;;
	    *)
		return ;;
	esac

	LEVEL=${1#RAID}

	for i in devcount devs sparecount sparedevs; do
		db_subst mdcfg/raid$i LEVEL "$LEVEL"
	done
	db_subst mdcfg/raiddevcount MINIMUM "$MIN_SIZE"

	db_set mdcfg/raiddevcount "$MIN_SIZE"

	# Get the count of active devices
	while [ $OK -eq 0 ]; do
		db_input critical mdcfg/raiddevcount
		db_go
		if [ $? -eq 30 ]; then
			return
		fi

		# Figure out, if the user entered a number
		db_get mdcfg/raiddevcount
		RET=$(echo $RET | sed -e "s/[[:space:]]//g")
		if [ "$RET" ]; then
			let "OK=${RET}>0 && ${RET}<99"
		fi
	done


	db_set mdcfg/raidsparecount "0"
	OK=0

	# Same procedure as above, but get the number of spare partitions
	# this time.
	# TODO: Make a general function for this kind of stuff
	while [ $OK -eq 0 ]; do
		db_input critical mdcfg/raidsparecount
		db_go
		if [ $? -eq 30 ]; then
			return
		fi
		db_get mdcfg/raidsparecount
		RET=$(echo $RET | sed -e "s/[[:space:]]//g")
		if [ "$RET" ]; then
			let "OK=${RET}>=0 && ${RET}<99"
		fi
	done

	db_get mdcfg/raiddevcount
	DEV_COUNT="$RET"
	if [ $LEVEL -ne 1 ]; then
		if [ $DEV_COUNT -lt $MIN_SIZE ]; then
			DEV_COUNT=$MIN_SIZE # Minimum number for the selected RAID level
		fi
	fi
	db_get mdcfg/raidsparecount
	SPARE_COUNT="$RET"
	REQUIRED=$(($DEV_COUNT + $SPARE_COUNT))
	if [ $LEVEL -ne 1 ]; then
		if [ $REQUIRED -gt $NUM_PART ]; then
			db_subst mdcfg/notenoughparts NUM_PART "$NUM_PART"
			db_subst mdcfg/notenoughparts REQUIRED "$REQUIRED"
			db_input critical mdcfg/notenoughparts
			db_go mdcfg/notenoughparts
			return
		fi
	fi

	db_set mdcfg/raiddevs ""
	SELECTED=0

	# Loop until the correct number of active devices has been selected for RAID 5, 6
	# Loop until at least one device has been selected for RAID 1
	until ([ $LEVEL -ne 1 ] && [ $SELECTED -eq $DEV_COUNT ]) || \
	      ([ $LEVEL -eq 1 ] && [ $SELECTED -gt 0 ] && [ $SELECTED -le $DEV_COUNT ]); do
		db_subst mdcfg/raiddevs COUNT "$DEV_COUNT"
		db_subst mdcfg/raiddevs PARTITIONS "$PARTITIONS"
		db_input critical mdcfg/raiddevs
		db_go
		if [ $? -eq 30 ]; then
			return
		fi

		db_get mdcfg/raiddevs
		SELECTED=0
		for i in $RET; do
			DEVICE=$(echo $i | sed -e "s/,//")
			let SELECTED++
		done
	done

	MISSING_DEVICES=""
	if [ $LEVEL -eq 1 ]; then
		# Add "missing" for as many devices as weren't selected
		while [ $SELECTED -lt $DEV_COUNT ]; do
			MISSING_DEVICES="$MISSING_DEVICES missing"
			let SELECTED++
		done
	fi

	# Remove partitions selected in raiddevs from the PARTITION list
	db_get mdcfg/raiddevs

	prune_partitions "$RET"

	db_set mdcfg/raidsparedevs ""
	SELECTED=0
	if [ $SPARE_COUNT -gt 0 ]; then
		FIRST=1
		# Loop until the correct number of devices has been selected.
		# That means any number less than or equal to the spare count.
		while [ $SELECTED -gt $SPARE_COUNT ] || [ $FIRST -eq 1 ]; do
			FIRST=0
			db_subst mdcfg/raidsparedevs COUNT "$SPARE_COUNT"
			db_subst mdcfg/raidsparedevs PARTITIONS "$PARTITIONS"
			db_input critical mdcfg/raidsparedevs
			db_go
			if [ $? -eq 30 ]; then
				return
			fi

			db_get mdcfg/raidsparedevs
			SELECTED=0
			for i in $RET; do
				DEVICE=$(echo $i | sed -e "s/,//")
				let SELECTED++
			done
		done
	fi

	# The number of spares the user has selected
	NAMED_SPARES=$SELECTED

	db_get mdcfg/raiddevs
	RAID_DEVICES=$(echo $RET | sed -e "s/,//g")

	db_get mdcfg/raidsparedevs
	SPARE_DEVICES=$(echo $RET | sed -e "s/,//g")

	MISSING_SPARES=""

	COUNT=$NAMED_SPARES
	while [ $COUNT -lt $SPARE_COUNT ]; do
		MISSING_SPARES="$MISSING_SPARES missing"
		let COUNT++
	done

	# Find the next available md-number
	MD_NUM=$(grep ^md /proc/mdstat | \
		 sed -e 's/^md\(.*\) : active .*/\1/' | sort | tail -n1)
	if [ -z "$MD_NUM" ]; then
		MD_NUM=0
	else
		let MD_NUM++
	fi

	logger -t mdcfg "Selected spare count: $NAMED_SPARES"
	logger -t mdcfg "Raid devices count: $DEV_COUNT"
	logger -t mdcfg "Spare devices count: $SPARE_COUNT"
	log-output -t mdcfg \
		mdadm --create /dev/md$MD_NUM --auto=yes --force -R -l raid${LEVEL} \
		      -n $DEV_COUNT -x $SPARE_COUNT $RAID_DEVICES \
		      $MISSING_DEVICES $SPARE_DEVICES $MISSING_SPARES
}

md_mainmenu() {
	while true; do
		db_set mdcfg/mainmenu false
		db_input critical mdcfg/mainmenu
		db_go
		if [ $? -eq 30 ]; then
			exit 30
		fi
		db_get mdcfg/mainmenu
		case $RET in
		    "Create MD device")
			md_createmain ;;
		    "Delete MD device")
			md_delete ;;
		    "Finish")
			break ;;
		esac
	done
}

### Main of script ###

# Load the modules and scan for MD devices if needed
if ! [ -e /proc/mdstat ]; then
	# Try to load the necesarry modules.
	depmod -a >/dev/null 2>&1
	modprobe md-mod >/dev/null 2>&1

	# Make sure that we have md-support
	if [ ! -e /proc/mdstat ]; then
		db_set mdcfg/nomd false
		db_input high mdcfg/nomd
		db_go
		exit 0
	fi

	# Try to detect MD devices, and start them
	# mdadm will fail if /dev/md does not already exist
	mkdir -p /dev/md

	log-output -t mdcfg --pass-stdout \
		mdadm --examine --scan --config=partitions >/tmp/mdadm.conf

	log-output -t mdcfg \
		mdadm --assemble --scan --run \
		--config=/tmp/mdadm.conf --auto=yes
fi

# Force mdadm to be installed on the target system
apt-install mdadm

# We want the "go back" button
#db_capb backup

md_mainmenu

exit 0
