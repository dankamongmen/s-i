#! /bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

MISSING_MODULES_LIST=""

# This is a gross and stupid hack, but we don't have a better idea right
# now. See Debian bug #136743
if [ -x /sbin/depmod ]; then
	depmod -a > /dev/null 2>&1 || true
fi

log () {
    logger -t hw-detect "$@"
}

is_not_loaded() {
    module="$1"
    if cut -d" " -f1 /proc/modules | grep -q "^${module}\$" ; then
	false
    else
	true
    fi
}

load_module() {
    module="$1"
    db_fset hw-detect/module_params seen false
    db_subst hw-detect/module_params MODULE "$module"
    db_input low hw-detect/module_params || [ $? -eq 30 ]
    db_go
    db_get hw-detect/module_params
    if modprobe -v "$module" $RET >> /var/log/messages 2>&1 ; then
	# Not sure if this is useful.  After all, 'discover' is installed
	# in /target/. [pere 2003-04-18]
	#prebaseconfig=/usr/lib/prebaseconfig.d/40ethdetect
	#echo "echo \"$module $RET\" >> /target/etc/modules" >> $prebaseconfig
	:
    else
	db_fset hw-detect/modprobe_error seen false
	db_subst hw-detect/modprobe_error CMD_LINE_PARAM "modprobe -v $module $RET"
	db_input medium hw-detect/modprobe_error || [ $? -eq 30 ]
	db_go
    fi
}

load_modules()
{
    old=`cat /proc/sys/kernel/printk`
    echo 0 > /proc/sys/kernel/printk
    for module in $* ; do
	load_module $module
    done
    echo $old > /proc/sys/kernel/printk
}

# wrapper for discover command that can distinguish Discover 1.x and 2.x
discover_hw () {
    DISCOVER=/sbin/discover
    if [ -f /usr/bin/didiscover ] ; then
        log "Testing experimental discover2 package."

        DISCOVER=/usr/bin/didiscover
    fi
    # Ugh, Discover 1.x didn't exit with nonzero status if given an
    # unrecongized option!
    DISCOVER_TEST=$($DISCOVER --version 2> /dev/null)
    if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
        # Discover 2.x, see <URL:http://hackers.progeny.com/discover/> for doc
        # XXX: This is copied from xfree86, and do not work yet

        log "Using discover2 do not work yet."

        dpath=linux/module/name
        dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
        dflags="-d all -e ata -e pci -e pcmcia -e scsi all"

        $DISCOVER --data-path=$dpath --data-version=$dver \
	    --data-vendor --data-model --format='%s:%s %s' $dflags
    else
        # must be Discover 1.x
        $DISCOVER --format="%m:%V %M\n" \
            --disable-all --enable=pci,ide,scsi,pcmcia scsi cdrom ethernet |
	  sed 's/ $//'
    fi
}

# Some pci chipsets are needed or there can be DMA or other problems.
get_ide_chipset_info() {
    for ide_module in /lib/modules/*/kernel/drivers/ide/pci/*.o; do
    	if [ -e $ide_module ]; then
		baseidemod=$(echo $ide_module | sed s/\.o$// | sed 's/.*\///')
		echo "$baseidemod:IDE chipset support"
    	fi
    done
}

# Return list of lines with "Kernel module<tab>Vendor<tab>Model"
get_hw_info() {
    # Try to make sure the floppy driver is available
    echo "floppy:Linux Floppy Driver"

    discover_hw

    # Manually load modules to enable things we can't detect.
    # XXX: This isn't the best way to do this; we should autodetect.
    # The order of these packages are important. [pere 2003-03-16]
    echo "ide-mod:Linux IDE driver"
    echo "ide-probe-mod:Linux IDE probe driver"

    get_ide_chipset_info

    echo "ide-detect:Linux IDE detection driver"
    echo "ide-floppy:Linux IDE floppy driver"
    echo "ide-disk:Linux ATA DISK driver"
    echo "ide-cd:Linux ATAPI CD-ROM driver"
    echo "isofs:Linux ISO 9660 filesystem driver"

    if [ -d /proc/bus/usb ]; then
    	echo "usb-storage:USB storage"
    fi
}


db_settitle hw-detect/title

log "Detecting hardware..."
# Put up a progress bar just to have something on screen if the hardware
# detection should hang.
db_progress START 0 2 hw-detect/detect_progress_title
db_progress INFO hw-detect/detect_progress_step
ALL_HW_INFO=$(get_hw_info)
db_progress STEP 1
# Remove modules that are already loaded, and count how many are left.
LOADED_MODULES=$(cat /proc/modules | cut -f 1 -d ' ')
count=0
# Setting IFS to adjust how the for loop splits the values
IFS_SAVE="$IFS"
IFS="
"
for device in $ALL_HW_INFO; do
    	module="`echo $device | cut -d: -f1`"
	loaded=0
	for m in $LOADED_MODULES; do
		if [ "$m" = "$module" ]; then
			loaded=1
			break
		fi
	done
	if [ "$loaded" = 0 ]; then
		count=$(expr $count + 1)
		HW_INFO="$HW_INFO
$device"
	fi
done
IFS="$IFS_SAVE"
db_progress STEP 1
db_progress STOP

if [ "$count" != 0 ]; then
	log "Loading modules..."
	db_progress START 0 $count hw-detect/load_progress_title
	IFS_SAVE="$IFS"
	IFS="
	"
	for device in $HW_INFO; do
	    module="`echo $device | cut -d: -f1`"
	    cardname="`echo $device | cut -d: -f2`"
	    # Restore IFS after extracting the fields.
	    IFS="$IFS_SAVE"
	
	    if [ -z "$module" ] ; then module="[Unknown]" ; fi
	    if [ -z "$cardname" ] ;   then cardname="[Unknown]" ; fi
	
	    log "Detected module '$module' for '$cardname'"
	
	    if [ "$module" != "ignore" -a "$module" != "[Unknown]" ]; then
	    	db_subst hw-detect/load_progress_step CARDNAME "$cardname"
	        db_subst hw-detect/load_progress_step MODULE "$module"
	        db_progress INFO hw-detect/load_progress_step
	        log "Trying to load module '$module'"
	
	        if find /lib/modules/`uname -r`/ | grep -q /${module}\\.o
	        then
	            if load_modules "$module"
	            then
	                :
	            else
	                log "Error loading driver '$module' for '$cardname'!"
		    fi
	        else
	       	    db_subst hw-detect/load_progress_skip_step CARDNAME "$cardname"
	            db_subst hw-detect/load_progress_skip_step MODULE "$module"
	            db_progress INFO hw-detect/load_progress_skip_step
	            log "Could not load driver '$module' for '$cardname'."
		    if [ -n "$MISSING_MODULES_LIST" ]; then
			    MISSING_MODULES_LIST="$MISSING_MODULES_LIST, "
		    fi
		    MISSING_MODULES_LIST="$MISSING_MODULES_LIST$module ($cardname)"
	        fi
	    fi
	
	    db_progress STEP 1

	    # XXX why is this ere? Paranioa?
	    IFS="
	"
	done
	IFS="$IFS_SAVE"
	
	db_progress STOP
fi
	
# always load sd_mod and sr_mod if a scsi controller module was loaded.
# sd_mod to find the disks, and sr_mod to find the CD-ROMs
if [ -e /proc/scsi/scsi ] ; then
    if grep -q "Attached devices: none" /proc/scsi/scsi ; then
        :
    else
	for module in sd_mod sr_mod; do
		if is_not_loaded "$module" ; then
			load_modules $module
		fi
	done
    fi
fi

if [ -n "$MISSING_MODULES_LIST" ]; then
	log "Missing modules '$MISSING_MODULES_LIST"
	# Tell the user to try to load more modules from floppy
	template=hw-detect/missing_modules
	db_fset "$template" seen false
	db_subst "$template" MISSING_MODULES_LIST "$MISSING_MODULES_LIST" || true
	db_input low "$template" || [ $? -eq 30 ]
	db_go || true
fi

# get pcmcia running if possible
if [ -x /etc/init.d/pcmcia ]; then
	/etc/init.d/pcmcia start </dev/null 2>&1 | logger -t hw-detect
fi

if [ -d /proc/bus/pccard ]; then
	# Ask for pcmcia-cs to be installed into target
	apt-install pcmcia-cs || true
fi

	    
# Ask for discover to be installed into /target/, to make sure the
# required drivers are loaded.
apt-install discover || true

exit 0
