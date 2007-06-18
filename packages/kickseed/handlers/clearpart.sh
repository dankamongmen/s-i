#! /bin/sh

clearpart_handler () {
	eval set -- "$(getopt -o '' -l linux,all,drives:,initlabel -- "$@")" || die_getopt clearpart
	while :; do
		case $1 in
			--linux)
				die "clearing all Linux partitions not supported yet"
				;;
			--all)
				ks_log "can't clear multiple drives; assuming just the first one"
				# TODO: this will break when we move to udev
				# device names, and I bet it isn't safe for
				# installs from USB ...
				ks_preseed d-i partman-auto/disk string /dev/discs/disc0/disc
				shift
				;;
			--drives)
				case $2 in
					*,*)
						die "clearing multiple drives not supported"
						;;
					*)
						ks_preseed d-i partman-auto/disk string "/dev/$OPTARG"
						;;
				esac
				shift 2
				;;
			--initlabel)
				ks_preseed d-i partman/confirm_write_new_label boolean true
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt clearpart ;;
		esac
	done
}
