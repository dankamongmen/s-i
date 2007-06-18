#! /bin/sh

mouse_handler () {
	if [ "$#" -eq 0 ]; then
		# autodetection is the default
		return
	fi

	eval set -- "$(getopt -o '' -l device:,emulthree -- "$@")" || { warn_getopt mouse; return; }
	while :; do
		case $1 in
			--device)
				ks_preseed xserver-xorg xserver-xorg/config/inputdevice/mouse/port 'select' "/dev/$2"
				shift 2
				;;
			--emulthree)
				ks_preseed xserver-xorg xserver-xorg/config/inputdevice/mouse/emulate3buttons boolean true
				shift
				;;
			--)	shift; break ;;
			*)	warn_getopt mouse; return ;;
		esac
	done

	# TODO: translate protocol into xserver-xorg's naming scheme
}
