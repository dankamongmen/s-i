
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`/bin/archdetect`" in
"m68k/amiga")
	amiga-fdisk $DISK
	;;
"m68k/atari")
	atari-fdisk $DISK
	;;
"m68k/mac")
	mac-fdisk $DISK
	;;
"m68k/bvme6000")
	pmac-fdisk $DISK
	;;
"m68k/mvme147")
	pmac-fdisk $DISK
	;;
"m68k/mvme16x")
	pmac-fdisk $DISK
	;;
*)
	fdisk $DISK
	;;
esac

exit $?

