
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`archdetect`" in
"powerpc/powermac_newworld")
	mac-fdisk $DISK
	;;
"powerpc/powermac_oldworld")
	mac-fdisk $DISK
	;;
"powerpc/chrp_pegasos")
	parted $DISK
	;;
"powerpc/chrp")
	fdisk $DISK
	;;
"powerpc/prep")
	fdisk $DISK
	;;
"powerpc/amiga")
	parted $DISK
	;;
*)
	fdisk $DISK
	;;
esac

exit $?

