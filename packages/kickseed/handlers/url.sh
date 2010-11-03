#! /bin/sh

url_handler () {
	eval set -- "$(getopt -o '' -l url: -- "$@")" || { warn_getopt url; return; }
	while :; do
		case $1 in
			--url)
				ks_preseed d-i mirror/country string manual
				case $2 in
					http://*/*)
						protocol=http
						;;
					ftp://*/*)
						protocol=ftp
						;;
					*)
						warn "URL type $2 not supported"
						continue
						;;
				esac
				ks_preseed d-i mirror/protocol 'select' "$protocol"
				# Disassemble URL into host and path.
				host="${2#$protocol://}"
				path="/${host#*/}"
				host="${host%%/*}"
				# Ensure trailing slash.
				if [ "${path%/}" = "$path" ]; then
					path="$path/"
				fi
				ks_preseed d-i "mirror/$protocol/hostname" string "$host"
				ks_preseed d-i "mirror/$protocol/directory" string "$path"
				# TODO: no support for specifying proxy?
				ks_preseed d-i "mirror/$protocol/proxy" string ''
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt url; return ;;
		esac
	done
}
