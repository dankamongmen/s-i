
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`/usr/bin/ppcdetect`" in
"NewWorld PowerMac"|"OldWorld PowerMac")
	mac-fdisk $DISK
	;;
"CHRP Pegasos")
	parted $DISK
	;;
"Amiga")
	parted $DISK
	;;
*)
	fdisk $DISK
	;;
esac

exit $?

