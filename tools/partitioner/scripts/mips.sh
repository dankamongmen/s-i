
DISK="$1"
[ -z "$DISK" ] && exit 1

# SGI dvh disklabels are best supported by fdisk.
fdisk $DISK

exit $?
