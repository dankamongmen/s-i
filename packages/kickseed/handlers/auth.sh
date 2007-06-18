#! /bin/sh

auth_handler () {
	# TODO --enablenis, --nisdomain=, --nisserver=, --enableldap,
	# --enableldapauth, --ldapserver=, --ldapbasedn=,
	# --enableldaptls, --enablekrb5, --krb5realm=, --krb5kdc=,
	# --krb5adminserver=, --enablehesiod, --hesiodlhs,
	# --hesiodrhs, --enablesmbauth, --smbservers=, --smbworkgroup=
	eval set -- "$(getopt -o '' -l enablemd5,useshadow,enableshadow,enablecache -- "$@")" || { warn_getopt auth; return; }
	while :; do
		case $1 in
			--enablemd5)
				ks_preseed passwd passwd/md5 boolean true
				shift
				;;
			--useshadow|--enableshadow)
				# TODO: this is true by default already
				ks_preseed passwd passwd/shadow boolean true
				shift
				;;
			--enablecache)
				warn "nscd not supported"
				shift
				;;
			--)	shift; break ;;
			*)	warn_getopt auth; return ;;
		esac
	done
}
