
DISK="$1"
[ -z "$DISK" ] && exit 1

case "$DEBIAN_FRONTEND" in
*)
	fdasd $DISK
	;;
esac

exit $?

