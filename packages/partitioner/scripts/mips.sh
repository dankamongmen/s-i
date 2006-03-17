
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`archdetect`" in
"mips/r4k-ip22")
        # SGI dvh disklabels are best supported by fdisk.
        fdisk $DISK
        ;;
"mips/r5k-ip22")
        # SGI dvh disklabels are best supported by fdisk.
        fdisk $DISK
        ;;
"mips/sb1-bcm91250a")
        # The SWARM is a normal ATX board with standard IDE/SCSI devices
        cfdisk $DISK
        ;;
"mips/sb1a-bcm91480b")
        # The BigSur is a normal ATX board with standard IDE/SCSI devices
        cfdisk $DISK
        ;;
*)
        fdisk $DISK
        ;;
esac

exit $?
