#! /bin/sh

user_handler () {
	makeuser=1
	crypted=
	password=

	eval set -- "$(getopt -o '' -l disabled,fullname:,iscrypted,password: -- "$@")" || { warn_getopt user; return; }
	while :; do
		case $1 in
			--disabled)
				makeuser=
				shift
				;;
			--fullname)
				ks_preseed passwd passwd/user-fullname string "$2"
				shift 2
				;;
			--iscrypted)
				crypted=1
				shift
				;;
			--password)
				password="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt user; return ;;
		esac
	done

	if [ "$makeuser" ]; then
		if [ $# -ne 1 ]; then
			warn "user command requires a username"
			return
		fi

		ks_preseed passwd passwd/make-user boolean true
		ks_preseed passwd passwd/username string "$1"

		if [ "$password" ]; then
			if [ "$crypted" ]; then
				# requires passwd >= 1:4.0.13-5
				ks_preseed passwd passwd/user-password-crypted password "$password"
			else
				ks_preseed passwd passwd/user-password password "$password"
				ks_preseed passwd passwd/user-password-again password "$password"
			fi
		fi
	else
		ks_preseed passwd passwd/make-user boolean false
	fi
}
