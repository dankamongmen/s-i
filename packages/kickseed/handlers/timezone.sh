#! /bin/sh

# requires base-config 2.61ubuntu18
timezone_handler () {
	utc=

	eval set -- "$(getopt -o '' -l utc -- "$@")" || die_getopt timezone
	while :; do
		case $1 in
			--utc)
				utc=1
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt timezone ;;
		esac
	done

	if [ $# -ne 1 ]; then
		die "timezone command requires a timezone"
	fi

	if [ "$utc" ]; then
		ks_preseed base-config tzconfig/gmt boolean true
	else
		ks_preseed base-config tzconfig/gmt boolean false
	fi
	ks_preseed base-config tzconfig/preseed_zone "$1"
}
