#! /bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

# This is a gross and stupid hack, but needed because of Xu's
# unwillingness to run depmod in kernel-image's postinst.  See Debian
# bug #136743

if [ -x /sbin/depmod ]; then
	depmod -a > /dev/null 2>&1 || true
fi

log () {
    log=/var/log/messages
    echo "$0: $@" >> $log
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
    db_subst hw-detect/module_params MODULE "$module"
    db_input low hw-detect/module_params || [ $? -eq 30 ]
    db_go
    db_get hw-detect/module_params
    if modprobe -v "$module" $RET >> /var/log/messages 2>&1 ; then
	# Not sure if this is usefull.  After all, 'discover' is installed
	# in /target/. [pere 2003-04-18]
	#prebaseconfig=/usr/lib/prebaseconfig.d/40ethdetect
	#echo "echo \"$module $RET\" >> /target/etc/modules" >> $prebaseconfig
	:
    else
	db_subst hw-detect/modprobe_error CMD_LINE_PARAM "modprobe -v $module $RET"
	db_input critical hw-detect/modprobe_error || [ $? -eq 30 ]
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
    # Ugh, Discover 1.x didn't exit with nonzero status if given an
    # unrecongized option!
    DISCOVER_TEST=$(discover --version 2> /dev/null)
    if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
        # Discover 2.x
        # XXX: This is copied from xfree86, and do not work yet

        log "Using discover2 do not work yet."

        dpath=linux/module/name
        dver=2.4 # Kernel version
        dflags="-t -d all -e ata -e pci -e pcmcia -e scsi display"

        VENDOR_MODEL_FILE=$(tempfile)
        DRIVER_FILE=$(tempfile)
        discover --type-summary $dflags > $VENDOR_MODEL_FILE
        discover --data-path=$dpath --data-version=$dver $dflags > $DRIVER_FILE
        paste $VENDOR_MODEL_FILE $DRIVER_FILE
        rm -f $VENDOR_MODEL_FILE $DRIVER_FILE
    else
        # must be Discover 1.x
        /sbin/discover --format="%m\t%V %M\n" \
            --disable-all --enable=pci,ide,scsi,pcmcia scsi cdrom |
	  sed 's/ $//'
    fi
}

# Return list of lines with "Kernel module<tab>Vendor<tab>Model"
get_hw_info() {
    # Try to make sure the floppy driver is available
    echo "floppy	Linux Floppy Driver"

    discover_hw

    # Manually load modules to enable things we can't detect.
    # XXX: This isn't the best way to do this; we should autodetect.
    # The order of these packages are important. [pere 2003-03-16]
    echo "ide-mod	Linux IDE Driver"
    echo "ide-probe-mod	Linux IDE probe Driver"
    echo "ide-disk	Linux ATA DISK Driver"
    echo "ide-cd	Linux ATAPI CD-ROM Driver"
    echo "isofs	Linux ISO 9660 Filesystem Driver"
    #echo "i82365	Linux Unknown"
    #echo "sr_mod	Linux Unknown"
    #echo "usb-storage	Linux Unknown"
}

log "Detecting hardware..."

db_title "Detecting HW and loading kernel modules"

set -- `get_hw_info | wc -l`
count="$1"
#log "Progress bar from 0 to $count"
db_progress START 0 $count hw-detect/progress_title

# Setting IFS to adjust how the for loop splits the values
IFS_SAVE="$IFS"
IFS="
"
for device in `get_hw_info`
do
    module="`echo $device | cut -f1`"
    cardname="`echo $device | cut -f2`"
    # Restore IFS after extracting the fields.
    IFS="$IFS_SAVE"

    if [ -z "$module" ] ; then module="[Unknown]" ; fi
    if [ -z "$cardname" ] ;   then cardname="[Unknown]" ; fi

    log "Detected load module '$module' for '$cardname'"

    db_subst hw-detect/progress_step CARDNAME "$cardname"
    db_subst hw-detect/progress_step MODULE "$module"

    db_progress STEP 1 hw-detect/progress_step

    if [ "$module" != "ignore" -a "$module" != "[Unknown]" ] &&
	is_not_loaded $module
    then
        log "Trying to load module '$module'"

        if find /lib/modules/`uname -r`/ | grep -q ${module}\\.o
        then
                :
        else
	    if [ ! -d /dev/floppy ] ; then
		log "error: Unable to load floppy device driver"
		log "error: no way to load extra modules from floppy"
	    else
		while true
		do
		    template=hw-detect/not_included
		    db_fset "$template" seen false || true
		    db_subst "$template" CARDNAME "$cardname" || true
		    db_subst "$template" MODULE "$module" || true
		    db_input medium "$template" || [ $? -eq 30 ]
		    db_go || true
		    db_get "$template" || true
		    mounted=0
		    if [ "$RET" != "true" ]
			then
			break
		    fi

		    if mount -t ext2 /dev/floppy/0 /mnt
		    then
			if [ -f /mnt/modules.tgz ]
			then
			    mounted=1
			    break
			fi
			umount /mnt
		    fi
		done
	    fi

            if [ "$mounted" = "1" ]
            then
                log "Copying modules.tgz from ext2 modules disk... "
                gunzip -c /mnt/modules.tgz | tar xf - ${module}.o
                umount /mnt
                # Make sure 'depmod -a' accepts the new module
                chown -R root /lib/modules
                depmod -a
            fi
        fi

        if find /lib/modules/`uname -r`/ | grep -q ${module}\\.o
        then
            if load_modules "$module"
            then
                :
            else
                log "Error loading driver '$module' for $vendor $model!"
	    fi
        else
            log "Could not locate driver '$module' for $vendor $model."
        fi
    fi
    IFS="
"
done
IFS="$IFS_SAVE"

#log "Progress bar stop"
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

# Hey, we're done

# Ask for discover to be installed into /target/, to make sure the
# required drivers are loaded.
apt-install discover || true

exit 0
