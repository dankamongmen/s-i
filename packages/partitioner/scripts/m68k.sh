
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`archdetect`" in
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
"m68k/q40")
	atari-fdisk $DISK
	;;
"m68k/sun")
	parted $DISK
	;;
"m68k/sun3x")
	parted $DISK
	;;
*)
	parted $DISK
	;;
esac

if [ "`uname -r | grep '^2.2.'`" != "" ] && [ -x /usr/bin/update-dev ]; then
	update-dev
fi

exit $?

