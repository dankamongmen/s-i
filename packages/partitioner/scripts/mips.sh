
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
"mips/sb1-swarm-bn")
        # The Broadcom is a normal ATX board with standard IDE/SCSI devices
        cfdisk $DISK
        ;;
*)
        fdisk $DISK
        ;;
esac

exit $?
