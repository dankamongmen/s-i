#! /bin/sh

rootpw_handler () {
	setpassword=1
	crypted=

	eval set -- "$(getopt -o '' -l disabled,iscrypted -- "$@")" || { warn_getopt rootpw; return; }
	while :; do
		case $1 in
			--disabled)
				setpassword=
				shift
				;;
			--iscrypted)
				crypted=1
				shift
				;;
			--)	shift; break ;;
			*)	warn_getopt rootpw; return ;;
		esac
	done

	if [ "$setpassword" ]; then
		if [ $# -ne 1 ]; then
			warn "rootpw command requires a password"
			return
		fi

		# requires user-setup 1.1
		ks_preseed d-i passwd/root-login boolean true
		if [ "$crypted" ] && [ "$1" ]; then
			# requires passwd 1:4.0.13-5
			ks_preseed passwd passwd/root-password-crypted password "$1"
		else
			# requires passwd 1:4.0.3-30.7ubuntu6
			ks_preseed passwd passwd/root-password password "$1"
			ks_preseed passwd passwd/root-password-again password "$1"
			if [ -z "$1" ]; then
				ks_preseed passwd passwd/root-password-empty note ''
			fi
		fi
	else
		# requires user-setup 1.1
		ks_preseed d-i passwd/root-login boolean false
		set -- ''
	fi
}
