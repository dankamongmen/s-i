#! /bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

MISSING_MODULES=""

# This is a gross and stupid hack, but needed because of Xu's
# unwillingness to run depmod in kernel-image's postinst.  See Debian
# bug #136743

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
    if is_not_loaded "$module" ; then
	:
    else
	log "Module $module is already loaded.  Ignoring load request."
	return
    fi

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
    # Some pci chipsets are needed or there can be DMA or other problems.
    for ide_module in /lib/modules/*/kernel/drivers/ide/pci/*.o; do
    	if [ -e $ide_module ]; then
		baseidemod=$(echo $ide_module | sed s/\.o$// | sed 's/.*\///')
		echo "$baseidemod:IDE chipset support"
    	fi
    done
    echo "ide-detect:Linux IDE detection driver"
    echo "ide-floppy:Linux IDE floppy driver"
    echo "ide-disk:Linux ATA DISK driver"
    echo "ide-cd:Linux ATAPI CD-ROM driver"
    echo "isofs:Linux ISO 9660 filesystem driver"
    #echo "i82365:Linux Unknown"
    #echo "sr_mod:Linux Unknown"
    #echo "usb-storage:Linux Unknown"
}


db_settitle hw-detect/title

log "Detecting hardware..."
# Put up a progress bar just to have something on screen if the hardware
# detection should hang.
db_progress START 0 1 hw-detect/detect_progress_title
db_progress INFO hw-detect/detect_progress_step
HW_INFO=$(get_hw_info)
db_progress STEP 1
count=0
# Setting IFS to adjust how the for loop splits the values
IFS_SAVE="$IFS"
IFS="
"
# HW_INFO must be unquoted in these loops.
for device in $HW_INFO; do
	count=$(expr $count + 1)
done
IFS="$IFS_SAVE"
db_progress STOP

log "Loading modules..."
db_progress START 0 $count hw-detect/load_progress_title
# Setting IFS to adjust how the for loop splits the values
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

    log "Detected load module '$module' for '$cardname'"

    db_subst hw-detect/load_progress_step CARDNAME "$cardname"
    db_subst hw-detect/load_progress_step MODULE "$module"

    db_progress INFO hw-detect/load_progress_step

    if [ "$module" != "ignore" -a "$module" != "[Unknown]" ] &&
	is_not_loaded $module
    then
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
            log "Could not load driver '$module' for '$cardname'."
	    if [ -n "$MISSING_MODULES" ]; then
		    MISSING_MODULES="$MISSING_MODULES, "
	    fi
	    MISSING_MODULES="$MISSING_MODULES$module ($cardname)"
        fi
    fi

    db_progress STEP 1

    # XXX why is this ere? Paranioa?
    IFS="
"
done
IFS="$IFS_SAVE"

db_progress STOP

# always load sd_mod and sr_mod if a scsi controller module was loaded.
# sd_mod to find the disks, and sr_mod to find the CD-ROMs
if [ -e /proc/scsi/scsi ] ; then
    if grep -q "Attached devices: none" /proc/scsi/scsi ; then
        :
    else
        load_modules sd_mod sr_mod
    fi
fi

# If PCMCIA was detected, load the PCMCIA drivers and load cardmgr
if [ -e /proc/bus/pccard ]
then
    pcmcia=0
    if [ -d /lib/modules/*/pcmcia ]
    then
	log "PCMCIA was detected on this system."
	pcmcia=1
    fi

    if [ "$pcmcia" = "1" ]
    then
	gunzip /etc/pcmcia/config.gz
	/sbin/cardmgr
    fi
fi

if [ -n "$MISSING_MODULES" ]; then
	# Tell the user to try to load more modules from floppy
	template=hw-detect/missing_modules
	db_fset "$template" seen false
	db_subst "$template" MISSING_MODULES "$MISSING_MODULES" || true
	db_input low "$template"
	db_go || true
fi
	    
# Ask for discover to be installed into /target/, to make sure the
# required drivers are loaded.
apt-install discover || true

exit 0
