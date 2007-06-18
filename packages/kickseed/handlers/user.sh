#! /bin/sh

user_handler () {
	eval set -- "$(getopt -o '' -l fullname:,iscrypted,password: -- "$@")" || die_getopt user
	while :; do
		case $1 in
			--fullname)
				ks_preseed passwd passwd/user-fullname string "$2"
				shift 2
				;;
			--iscrypted)
				# requires passwd 1:4.0.3-30.7ubuntu7
				ks_preseed passwd passwd/user-password-crypted boolean true
				shift
				;;
			--password)
				ks_preseed passwd passwd/user-password password "$2"
				ks_preseed passwd passwd/user-password-again password "$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	die_getopt user ;;
		esac
	done

	if [ $# -ne 1 ]; then
		die "user command requires a username"
	fi

	ks_preseed passwd passwd/username string "$1"
}
