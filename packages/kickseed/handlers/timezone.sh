#! /bin/sh

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
		# requires clock-setup
		ks_preseed d-i clock-setup/utc boolean true
	else
		# requires clock-setup
		ks_preseed d-i clock-setup/utc boolean false
	fi
	# requires tzsetup
	ks_preseed d-i time/zone string "$1"
}
