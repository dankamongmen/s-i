#! /bin/sh

preseed_handler () {
	owner=d-i

	eval set -- "$(getopt -o '' -l owner: -- "$@")" || { warn_getopt preseed; return; }
	while :; do
		case $1 in
			--owner)
				owner="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt preseed; return ;;
		esac
	done

	if [ $# -ne 2 ] && [ $# -ne 3 ]; then
		warn "preseed command requires key, type, and optional value"
		return
	fi

	ks_preseed "$owner" "$1" "$2" "$3"
}
