#! /bin/sh

mouse_handler () {
	if [ "$#" -eq 0 ]; then
		# autodetection is the default
		return
	fi

	eval set -- "$(getopt -o '' -l device:,emulthree -- "$@")" || die_getopt mouse
	while :; do
		case $1 in
			--device)
				preseed xserver-xorg xserver-xorg/config/inputdevice/mouse/port 'select' "/dev/$2"
				shift 2
				;;
			--emulthree)
				preseed xserver-xorg xserver-xorg/config/inputdevice/mouse/emulate3buttons boolean true
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt mouse ;;
		esac
	done

	# TODO: translate protocol into xserver-xorg's naming scheme
}
