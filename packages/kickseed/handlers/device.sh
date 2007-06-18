#! /bin/sh

device_handler () {
	# type argument (args[0]) ignored
	# modulename="$1"
	eval set -- "$(getopt -o '' -l opts: -- "$@")" || die_getopt device
	while :; do
		case $1 in
			--opts)
				die "device --opts not implemented"
				;;
			--)	shift; break ;;
			*)	die_getopt device ;;
		esac
	done
}
