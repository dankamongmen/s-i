
DISK="$1"
[ -z "$DISK" ] && exit 1

case "$DEBIAN_FRONTEND" in
"slang")
	LANG=C cfdisk $DISK
	;;
"newt")
	LANG=C cfdisk $DISK
	;;
*)
	fdisk $DISK
	;;
esac

exit $?

