#! /bin/sh

auth_handler () {
	# TODO --enableldap, --enableldapauth, --ldapserver=, --ldapbasedn=,
	# --enableldaptls, --enablekrb5, --krb5realm=, --krb5kdc=,
	# --krb5adminserver=, --enablehesiod, --hesiodlhs,
	# --hesiodrhs, --enablesmbauth, --smbservers=, --smbworkgroup=
	eval set -- "$(getopt -o '' -l enablemd5,enablenis,nisdomain:,nisserver:,useshadow,enableshadow,enablecache -- "$@")" || { warn_getopt auth; return; }
	while :; do
		case $1 in
			--enablemd5)
				ks_preseed passwd passwd/md5 boolean true
				shift
				;;
			--enablenis)
				mkdir -p "$POSTSPOOL/auth.handler"
				touch "$POSTSPOOL/auth.handler/nis"
				shift
				;;
			--nisdomain)
				mkdir -p "$POSTSPOOL/auth.handler"
				echo "$2" > "$POSTSPOOL/auth.handler/nisdomain"
				shift 2
				;;
			--nisserver)
				mkdir -p "$POSTSPOOL/auth.handler"
				echo "$2" > "$POSTSPOOL/auth.handler/nisserver"
				shift 2
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

auth_post () {
	if [ -f "$POSTSPOOL/auth.handler/nisdomain" ]; then
		cp "$POSTSPOOL/auth.handler/nisdomain" /target/etc/defaultdomain
	fi

	if [ -e "$POSTSPOOL/auth.handler/nis" ]; then
		sed -i '/^\(passwd\|group\|shadow\):/s/$/ nis/;
			/^hosts:/s/files/files nis/;
			/^\(protocols\|services\):/s/$/ nis/' \
			/target/etc/nsswitch.conf
		apt-install nis || true
	fi

	# Must come after installing nis, since /etc/yp.conf is a conffile.
	if [ -f "$POSTSPOOL/auth.handler/nisserver" ]; then
		nisserver="$(cat "$POSTSPOOL/auth.handler/nisserver")"
		echo "ypserver $nisserver" >> /target/etc/yp.conf
	fi
}
