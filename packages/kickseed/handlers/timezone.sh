#! /bin/sh

# requires base-config 2.61ubuntu18
timezone_handler () {
	utc=

	eval set -- "$(getopt -o '' -l utc -- "$@")" || { warn_getopt timezone; return; }
	while :; do
		case $1 in
			--utc)
				utc=1
				shift
				;;
			--)	shift; break ;;
			*)	warn_getopt timezone; return ;;
		esac
	done

	if [ $# -ne 1 ]; then
		warn "timezone command requires a timezone"
		return
	fi

	if [ "$utc" ]; then
		ks_preseed base-config tzconfig/gmt boolean true
		# requires clock-setup
		ks_preseed d-i clock-setup/utc boolean true
	else
		ks_preseed base-config tzconfig/gmt boolean false
		# requires clock-setup
		ks_preseed d-i clock-setup/utc boolean false
	fi
	ks_preseed base-config tzconfig/preseed_zone string "$1"
	# requires tzsetup
	ks_preseed d-i time/zone string "$1"
}
