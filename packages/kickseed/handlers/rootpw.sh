#! /bin/sh

rootpw_handler () {
	eval set -- "$(getopt -o '' -l iscrypted -- "$@")" || die_getopt rootpw
	while :; do
		case $1 in
			--iscrypted)
				die "pre-crypted passwords not yet supported"
				;;
			--)	shift; break ;;
			*)	die_getopt rootpw ;;
		esac
	done

	if [ $# -ne 1 ]; then
		die "rootpw command requires a password"
	fi

	# requires passwd 1:4.0.3-30.7ubuntu4
	ks_preseed passwd passwd/root-password password "$1"
	ks_preseed passwd passwd/root-password-again password "$1"
	if [ -z "$1" ]; then
		ks_preseed passwd passwd/root-password-empty note ''
	fi
}
