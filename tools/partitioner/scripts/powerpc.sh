
DISK="$1"
[ -z "$DISK" ] && exit 1

case "$DEBIAN_FRONTEND" in
*)
	mac-fdisk $DISK
	;;
esac

exit $?

