#! /bin/sh

bootloader_handler_common () {
	useLilo="$1"
	shift
	# TODO --linear, --nolinear, --lba32
	eval set -- "$(getopt -o '' -l location:,password:,md5pass:,useLilo,upgrade -- "$@")" || { warn_getopt bootloader; return; }
	while :; do
		case $1 in
			--location)
				case $2 in
					mbr)
						# TODO: not always hd0; lilo
						ks_preseed d-i grub-installer/bootdev string '(hd0)'
						;;
					partition)
						# TODO: lilo
						ks_preseed d-i grub-installer/bootdev string '(hd0,1)'
						;;
					none)
						# TODO: need lilo-installer/skip too
						ks_preseed d-i grub-installer/skip boolean true
						;;
					*)
						warn_bad_arg bootloader location "$2"
						;;
				esac
				shift 2
				;;
			--password)
				# requires grub-installer 1.09
				ks_preseed d-i grub-installer/password password "$2"
				shift 2
				;;
			--md5pass)
				# requires grub-installer 1.31
				ks_preseed d-i grub-installer/password-crypted password "$2"
				shift 2
				;;
			--useLilo)
				useLilo=1
				shift
				;;
			--upgrade)
				warn "upgrades using installer not supported"
				;;
			--)	shift; break ;;
			*)	warn_getopt bootloader; return ;;
		esac
	done

	if [ "$useLilo" = 1 ]; then
		ks_preseed d-i grub-installer/skip boolean true
	fi
}

bootloader_handler () {
	bootloader_handler_common 0 "$@"
}

lilo_handler () {
	bootloader_handler_common 1 "$@"
}
