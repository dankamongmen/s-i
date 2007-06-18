#! /bin/sh

bootloader_handler_common () {
	useLilo="$1"
	shift
	# TODO --password=, --md5pass=, --linear, --nolinear, --lba32
	eval set -- "$(getopt -o '' -l location:,useLilo,upgrade -- "$@")" || die_getopt bootloader
	while :; do
		case $1 in
			--location)
				case $2 in
					mbr)
						# TODO: not always hd0; lilo
						preseed d-i grub-installer/bootdev string '(hd0)'
						;;
					partition)
						# TODO: lilo
						preseed d-i grub-installer/bootdev string '(hd0,1)'
						;;
					none)
						# TODO: need lilo-installer/skip too
						preseed d-i grub-installer/skip boolean true
						;;
					*)
						die_bad_arg bootloader location "$OPTARG"
						;;
				esac
				shift 2
				;;
			--useLilo)
				useLilo=1
				shift
				;;
			--upgrade)
				die "upgrades using installer not supported"
				;;
			--)	shift; break ;;
			*)	die_getopt bootloader ;;
		esac
	done

	if [ "$useLilo" = 1 ]; then
		preseed d-i grub-installer/skip boolean true
	fi
}

bootloader_handler () {
	bootloader_handler_common 0 "$@"
}

lilo_handler () {
	bootloader_handler_common 1 "$@"
}
