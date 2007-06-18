#! /bin/sh

rootpw_handler () {
	setpassword=1

	eval set -- "$(getopt -o '' -l disabled,iscrypted -- "$@")" || die_getopt rootpw
	while :; do
		case $1 in
			--disabled)
				setpassword=
				shift
				;;
			--iscrypted)
				# requires passwd 1:4.0.3-30.7ubuntu7
				ks_preseed passwd passwd/root-password-crypted boolean true
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt rootpw ;;
		esac
	done

	if [ "$setpassword" ]; then
		if [ $# -ne 1 ]; then
			die "rootpw command requires a password"
		fi
	else
		set -- ''
	fi

	# requires passwd 1:4.0.3-30.7ubuntu6
	ks_preseed passwd passwd/root-password password "$1"
	ks_preseed passwd passwd/root-password-again password "$1"
	if [ -z "$1" ]; then
		ks_preseed passwd passwd/root-password-empty note ''
	fi
}
