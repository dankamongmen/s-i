
DISK="$1"
[ -z "$DISK" ] && exit 1

case "`archdetect`" in
"arm/rpc")
        fdisk $DISK
        ;;
*)
        fdisk $DISK
        ;;
esac

exit $?
