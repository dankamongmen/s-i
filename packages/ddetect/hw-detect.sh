#! /bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

if [ -z "$1" ]; then
	PROGRESSBAR=hw-detect/detect_progress_step
else
	PROGRESSBAR=$1
fi

NEWLINE="
"
MISSING_MODULES_LIST=""

# This is a hack, but we don't have a better idea right now.
# See Debian bug #136743
if [ -x /sbin/depmod ]; then
	depmod -a > /dev/null 2>&1 || true
fi

log () {
	logger -t hw-detect "$@"
}

is_not_loaded() {
	! (cut -d" " -f1 /proc/modules | grep -q "^$1\$")
}

snapshot_devs() {
	echo -n `grep : /proc/net/dev | sort | cut -d':' -f1`
}

compare_devs() {
	local olddevs="$1"
	local devs="$2"
	echo "${devs#$olddevs}" | sed -e 's/^ //'
}

load_module() {
	local module="$1"
	local cardname="$2"
	local devs=""
	local olddevs=""
	local newdev=""
    
	old=`cat /proc/sys/kernel/printk`
	echo 0 > /proc/sys/kernel/printk
    
	db_fset hw-detect/module_params seen false
	db_subst hw-detect/module_params MODULE "$module"
	db_input low hw-detect/module_params || [ $? -eq 30 ]
	db_go
	db_get hw-detect/module_params
	devs="$(snapshot_devs)"
	if modprobe -v "$module" "$RET" >> /var/log/messages 2>&1 ; then
		if [ "$RET" != "" ]; then
			register-module "$module" "$RET"
		fi
	
		olddevs="$devs"
		devs="$(snapshot_devs)"
		newdevs="$(compare_devs "$olddevs" "$devs")"

		if [ -n "$newdevs" -a -n "$cardname" ]; then
			mkdir -p /etc/network
			for dev in $newdevs; do
				echo "${dev}:${cardname}" >> /etc/network/devnames
			done
		fi
	else   
		log "Error loading '$module'"
		if [ "$module" != floppy ] && [ "$module" != ide-floppy ]; then
			db_fset hw-detect/modprobe_error seen false
			db_subst hw-detect/modprobe_error CMD_LINE_PARAM "modprobe -v $module"
			db_input medium hw-detect/modprobe_error || [ $? -eq 30 ]
			db_go
		fi
	fi
    
	echo $old > /proc/sys/kernel/printk
}

# join hack for discover 2
dumb_join_discover (){
	IFS_SAVE="$IFS"
	IFS="$NEWLINE"
	for i in $MODEL_INFOS; do
		echo $1:$i;
		shift
	done
	IFS="$IFS_SAVE"
}

# wrapper for discover command that can distinguish Discover 1.x and 2.x
discover_hw () {
	DISCOVER=/sbin/discover
	if [ -f /usr/bin/discover ] ; then
		DISCOVER=/usr/bin/discover
	fi
	# Ugh, Discover 1.x didn't exit with nonzero status if given an
	# unrecongized option!
	DISCOVER_TEST=$($DISCOVER --version 2> /dev/null)
	if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
		log "Testing experimental discover2 package."
		dpath=linux/module/name
		dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
		dflags="-d all -e ata -e pci -e pcmcia -e \
			scsi bridge broadband fixeddisk humaninput modem \
			network optical removabledisk"

		MODEL_INFOS=$($DISCOVER -t $dflags)
		MODULES=$($DISCOVER --data-path=$dpath --data-version=$dver $dflags)
		dumb_join_discover $MODULES
	else
		# must be Discover 1.x
		$DISCOVER --format="%m:%V %M\n" --disable-all \
		          --enable=pci,ide,scsi,pcmcia scsi cdrom ethernet |
			sed 's/ $//'
	fi
}

# Some pci chipsets are needed or there can be DMA or other problems.
get_ide_chipset_info() {
	for ide_module in $(find /lib/modules/*/kernel/drivers/ide/pci/ -type f 2>/dev/null); do
		if [ -e $ide_module ]; then
			baseidemod=$(echo $ide_module | sed 's/\.o$//' | sed 's/\.ko$//' | sed 's/.*\///')
			echo "$baseidemod:IDE chipset support"
		fi
	done
}

# Return list of lines formatted "module:Description"
get_all_hw_info() {
	discover_hw
	if [ -d /proc/bus/usb ]; then
		echo "usb-storage:USB storage"
	fi
	get_manual_hw_info
}
   
# Manually load modules to enable things we can't detect.
# XXX: This isn't the best way to do this; we should autodetect.
# The order of these modules are important.
get_manual_hw_info() {
	echo "floppy:Linux Floppy"
	# ide-mod and ide-probe-mod are needed for older (2.4.20) kernels
	echo "ide-mod:Linux IDE driver"
	echo "ide-probe-mod:Linux IDE probe driver"
	get_ide_chipset_info
	echo "ide-detect:Linux IDE detection" # 2.4.x > 20
	echo "ide-generic:Linux IDE support" # 2.6
	echo "ide-floppy:Linux IDE floppy"
	echo "ide-disk:Linux ATA DISK"
	echo "ide-cd:Linux ATAPI CD-ROM"
	echo "isofs:Linux ISO 9660 filesystem"
	if [ "`udpkg --print-architecture`" = powerpc ]; then
		if [ -f /proc/device-tree/aliases/mac-io ]; then
			if [ -e "/proc/device-tree`cat /proc/device-tree/aliases/mac-io`/radio" ]; then
				echo "airport:Airport wireless"
			fi
		fi
	fi
}

# Should be greater than the number of kernel modules we can reasonably
# expect it will ever need to load.
MAX_STEPS=1000
OTHER_STEPS=5
# Use 1/10th of the progress bar for the non-module-load steps.
OTHER_STEPSIZE=$(expr $MAX_STEPS / 10 / $OTHER_STEPS)
db_progress START 0 $MAX_STEPS $PROGRESSBAR

log "Detecting hardware..."
db_progress INFO hw-detect/detect_progress_step
MANUAL_HW_INFO=$(get_manual_hw_info)
ALL_HW_INFO=$(get_all_hw_info)
db_progress STEP $OTHER_STEPSIZE

# Remove modules that are already loaded, and construct the list for the
# question.
LOADED_MODULES=$(cat /proc/modules | cut -f 1 -d ' ')
LIST=""
IFS_SAVE="$IFS"
IFS="$NEWLINE"
for device in $ALL_HW_INFO; do
	module="`echo $device | cut -d: -f1`"
	cardname="`echo $device | cut -d: -f2 | sed 's/,/ /g'`"
	loaded=0
	for m in $LOADED_MODULES; do
		if [ "$m" = "$module" ]; then
			loaded=1
			break
		fi
	done
	if [ "$loaded" = 0 ] &&
	   echo "$LIST" | grep -v -q '\(^\|, \)'"$module " ; then
		if [ -n "$LIST" ]; then
			LIST="$LIST, "
		fi
		if [ -z "$module" ] ; then module="[Unknown]" ; fi
		if [ -z "$cardname" ] ; then cardname="[Unknown]" ; fi
		# If this is changed, be sure to change the loop
		# that parses it, below!
		LIST="$LIST$module ($cardname)"
	fi
done
IFS="$IFS_SAVE"
db_progress STEP $OTHER_STEPSIZE

# Ask which modules to install.
db_subst hw-detect/select_modules list "$LIST"
db_set hw-detect/select_modules "$LIST"
db_fset hw-detect/select_modules seen false
db_input medium hw-detect/select_modules || true
db_go || exit 10 # back up
db_get hw-detect/select_modules
LIST="$RET"

list_to_lines() {
	echo "$LIST" | sed 's/, /\n/g'
}

# Work out amount to step per module load. expr rounds down, so 
# it may not get quite to 100%, but will at least never exceed it.
MODULE_STEPSIZE=$(expr \( $MAX_STEPS - \( $OTHER_STEPS \* $OTHER_STEPSIZE \) \) / $(list_to_lines | wc -l))

log "Loading modules..."
IFS="$NEWLINE"

# Save from being called so many times
MODPATH="/lib/modules/$(uname -r)/"
for device in $(list_to_lines); do
	module="`echo $device | cut -d' ' -f1`"
	cardname="`echo $device | cut -d'(' -f2 | sed 's/)$//'`"
	# Restore IFS after extracting the fields.
	IFS="$IFS_SAVE"

	if [ -z "$module" ] ; then module="[Unknown]" ; fi
	if [ -z "$cardname" ] ; then cardname="[Unknown]" ; fi

	log "Detected module '$module' for '$cardname'"

	if [ "$module" != "ignore" -a "$module" != "[Unknown]" ] && \
	   is_not_loaded "$module"; then
		if find $MODPATH | grep -q /${module}\\. ; then
			db_subst hw-detect/load_progress_step CARDNAME "$cardname"
			db_subst hw-detect/load_progress_step MODULE "$module"
			db_progress INFO hw-detect/load_progress_step
			log "Trying to load module '$module'"
			if [ "$cardname" = "[Unknown]" ]; then
				load_module "$module"
			else
				load_module "$module" "$cardname"
			fi
		else
			db_subst hw-detect/load_progress_skip_step CARDNAME "$cardname"
			db_subst hw-detect/load_progress_skip_step MODULE "$module"
			db_progress INFO hw-detect/load_progress_skip_step
			log "Missing module '$module'."
			# Only add the module to the missing list if it was not
			# manually added to the list of modules to load.
			if ! echo "$MANUAL_HW_INFO" | grep -q "$module:" &&
			   ! echo "$MISSING_MODULES_LIST" | grep -q '\(^\|, \)'"$module " ; then
				if [ -n "$MISSING_MODULES_LIST" ]; then
					MISSING_MODULES_LIST="$MISSING_MODULES_LIST, "
				fi
				MISSING_MODULES_LIST="$MISSING_MODULES_LIST$module ($cardname)"
			fi
		fi
	fi

	db_progress STEP $MODULE_STEPSIZE
	IFS="$NEWLINE"
done
IFS="$IFS_SAVE"

# always load sd_mod and sr_mod if a scsi controller module was loaded.
# sd_mod to find the disks, and sr_mod to find the CD-ROMs
if [ -e /proc/scsi/scsi ] && ! grep -q "Attached devices: none" /proc/scsi/scsi; then
	if grep -q 'Type:[ ]\+Direct-Access' /proc/scsi/scsi && \
	   is_not_loaded "sd_mod" && \
	   ! grep -q '^[^[:alpha:]]\+sd$' /proc/devices; then
		db_subst hw-detect/load_progress_step CARDNAME "SCSI disk support"
		db_subst hw-detect/load_progress_step MODULE "sd_mod"
		db_progress INFO hw-detect/load_progress_step
		load_module sd_mod
		register-module sd_mod
	fi
	db_progress STEP $OTHER_STEPSIZE
	if grep -q 'Type:[ ]\+CD-ROM' /proc/scsi/scsi && \
	   is_not_loaded "sr_mod" &&
	   ! grep -q '^[^[:alpha:]]\+sr$' /proc/devices; then
		db_subst hw-detect/load_progress_step CARDNAME "SCSI CDROM support"
		db_subst hw-detect/load_progress_step MODULE "sr_mod"
		db_progress INFO hw-detect/load_progress_step
		load_module sr_mod
		register-module sr_mod
	fi
	db_progress STEP $OTHER_STEPSIZE
fi

# if there is an ide bus, then register the ide CD modules so they'll be
# available on the target system for base-config
if [ -e /proc/ide/ -a "`find /proc/ide/* -type d 2>/dev/null`" != "" ]; then
	register-module ide-cd
	register-module ide-detect
fi

# get pcmcia running if possible
if [ -x /etc/init.d/pcmcia ] && \
#   db_input medium hw-detect/start_pcmcia && db_go && \ # not asked yet
   db_get hw-detect/start_pcmcia && \
   [ "$RET" = true ]; then
	db_progress INFO hw-detect/pcmcia_step

	# If hotplugging is available in the kernel, we can use it to load
	# modules for Cardbus cards and tell which network interfaces belong
	# to PCMCIA devices. The former is only necessary on 2.4 kernels,
	# though.
	if [ -f /proc/sys/kernel/hotplug ]; then
		# Snapshot discover information so we can detect modules for
		# Cardbus cards by later comparison in the hotplug handler.
		# (Only on 2.4 kernels.)
		if expr `uname -r` : "2.4.*" >/dev/null 2>&1; then
			DISCOVER_TEST=$(discover --version 2> /dev/null)
			if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
				dpath=linux/module/name
				dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
				dflags="-d all -e pci scsi fixeddisk modem network removabledisk"
		
				echo `discover --data-path=$dpath --data-version=$dver $dflags` \
					| sed 's/ $//' >/tmp/pcmcia-discover-snapshot
			else
				discover --format="%m " --disable-all --enable=pci \
					scsi ide ethernet \
					| sed 's/ $//' >/tmp/pcmcia-discover-snapshot
			fi
		fi
	
		# Simple handling of hotplug events during PCMCIA detection
		saved_hotplug=`cat /proc/sys/kernel/hotplug`
		echo /bin/hotplug-pcmcia >/proc/sys/kernel/hotplug
	fi
    
	CARDMGR_OPTS="-f" /etc/init.d/pcmcia start </dev/null 2>&1 \
		| logger -t hw-detect
    
	if [ -f /proc/sys/kernel/hotplug ]; then
		echo $saved_hotplug >/proc/sys/kernel/hotplug
		rm -f /tmp/pcmcia-discover-snapshot
	fi
    
	db_progress STEP $OTHER_STEPSIZE
fi
if [ -e /proc/bus/pccard/drivers ]; then
	log "Detected PCMCIA, installing pcmcia-cs."
	apt-install pcmcia-cs || true
fi

# Ask for discover to be installed into /target/, to make sure the
# required drivers are loaded.
apt-install discover || true

# Install hotplug as well (for USB, IEEE1394, CardBus, and some SCSI)
if [ -f /proc/sys/kernel/hotplug ]; then 
	log "Detected hotplug support, installing hotplug."
	apt-install hotplug || true
fi

db_progress SET $MAX_STEPS
db_progress STOP

if [ -n "$MISSING_MODULES_LIST" ]; then
	log "Missing modules '$MISSING_MODULES_LIST"
	# Tell the user to try to load more modules from floppy
	template=hw-detect/missing_modules
	db_fset "$template" seen false
	db_subst "$template" MISSING_MODULES_LIST "$MISSING_MODULES_LIST" || true
	db_input low "$template" || [ $? -eq 30 ]
	db_go || true
fi

exit 0
