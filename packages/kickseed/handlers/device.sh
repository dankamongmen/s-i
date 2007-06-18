#! /bin/sh

device_handler () {
	opts=
	eval set -- "$(getopt -o '' -l opts: -- "$@")" || { warn_getopt device; return; }
	while :; do
		case $1 in
			--opts)
				opts="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt device; return ;;
		esac
	done

	if [ "$opts" ]; then
		if [ $# -ne 2 ]; then
			warn "device command requires type and modulename"
			return
		fi
		# type argument ($1) ignored
		modulename="$2"

		# requires hw-detect 1.17
		ks_preseed d-i "hw-detect/module_params/$2" string "$opts"
	fi
}
