
DISK="$1"
[ -z "$DISK" ] && exit 1

# Load srm_env.o if we can; this should fail on ARC-based systems.
(modprobe srm_env || true) 2> /dev/null

# Have SRM, so need BSD disklabels -- always use fdisk.
if [ -f /proc/srm_environment/named_variables/booted_dev ]; then
	fdisk $DISK
	exit $?
fi

# Otherwise, pick based on the frontend.
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

