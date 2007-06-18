#! /bin/sh

clearpart_handler () {
	# TODO: --linux, --all
	eval set -- "$(getopt -o '' -l drives:,initlabel -- "$@")" || die_getopt clearpart
	while :; do
		case $1 in
			--drives)
				case $2 in
					*,*)
						die "clearing multiple drives not supported"
						;;
					*)
						preseed d-i partman-auto/disk string "/dev/$OPTARG"
						;;
				esac
				shift 2
				;;
			--initlabel)
				preseed d-i partman-auto/confirm_write_new_label boolean true
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt clearpart ;;
		esac
	done
}
