
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`archdetect`" in
"mipsel/r3k-kn02")
        fdisk $DISK
        ;;
"mipsel/r4k-kn04")
        fdisk $DISK
        ;;
"mipsel/sb1-swarm-bn")
        # The Broadcom is a normal ATX board with standard IDE/SCSI devices
        cfdisk $DISK
        ;;
"mipsel/cobalt")
        cfdisk $DISK
        ;;
*)
        fdisk $DISK
        ;;
esac

exit $?
