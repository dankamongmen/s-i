#! /bin/sh

auth_handler () {
	# TODO --enablenis, --nisdomain=, --nisserver=, --enableldap,
	# --enableldapauth, --ldapserver=, --ldapbasedn=,
	# --enableldaptls, --enablekrb5, --krb5realm=, --krb5kdc=,
	# --krb5adminserver=, --enablehesiod, --hesiodlhs,
	# --hesiodrhs, --enablesmbauth, --smbservers=, --smbworkgroup=
	eval set -- "$(getopt -o '' -l enablemd5,useshadow,enableshadow,enablecache -- "$@")" || die_getopt auth
	while :; do
		case $1 in
			--enablemd5)
				preseed passwd passwd/md5 boolean true
				shift
				;;
			--useshadow|--enableshadow)
				# TODO: this is true by default already
				preseed passwd passwd/shadow boolean true
				shift
				;;
			--enablecache)
				die "nscd not supported"
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt auth ;;
		esac
	done
}
