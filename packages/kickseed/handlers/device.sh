#! /bin/sh

device_handler () {
	# type argument (args[0]) ignored
	# modulename="$1"
	eval set -- "$(getopt -o '' -l opts: -- "$@")" || { warn_getopt device; return; }
	while :; do
		case $1 in
			--opts)
				warn "device --opts not implemented"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt device; return ;;
		esac
	done
}
