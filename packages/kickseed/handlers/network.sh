#! /bin/sh

network_handler () {
	got_ipaddress=
	got_gateway=
	got_nameservers=
	got_netmask=
	eval set -- "$(getopt -o '' -l bootproto:,device:,ip:,gateway:,nameserver:,nodns,netmask:,hostname: -- "$@")" || { warn_getopt network; return; }
	while :; do
		case $1 in
			--bootproto)
				case $2 in
					dhcp|bootp)
						;;
					static)
						ks_preseed d-i netcfg/disable_dhcp boolean true
						;;
					*)
						warn_bad_arg network bootproto "$2"
						;;
				esac
				shift 2
				;;
			--device)
				ks_preseed d-i netcfg/choose_interface 'select' "$2"
				shift 2
				;;
			--ip)
				ks_preseed d-i netcfg/get_ipaddress string "$2"
				got_ipaddress=1
				shift 2
				;;
			--gateway)
				ks_preseed d-i netcfg/get_gateway string "$2"
				got_gateway=1
				shift 2
				;;
			--nameserver)
				ks_preseed d-i netcfg/get_nameservers string "$2"
				got_nameservers=1
				shift 2
				;;
			--nodns)
				ks_preseed d-i netcfg/get_nameservers string ''
				got_nameservers=1
				shift
				;;
			--netmask)
				ks_preseed d-i netcfg/get_netmask string "$2"
				got_netmask=1
				shift 2
				;;
			--hostname)
				ks_preseed d-i netcfg/get_hostname string "$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt network; return ;;
		esac
	done

	# If all the information displayed on the netcfg/confirm_static
	# screen was preseeded, skip it.
	if [ "$got_ipaddress" ] && [ "$got_netmask" ] && \
	   [ "$got_gateway" ] && [ "$got_nameservers" ]; then
		ks_preseed d-i netcfg/confirm_static boolean true
	fi
}
