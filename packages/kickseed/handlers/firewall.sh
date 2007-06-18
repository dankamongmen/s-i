#! /bin/sh

firewall_handler () {
	eval set -- "$(getopt -o '' -l high,medium,enabled,enable,disabled,disable,trust:,dhcp,ssh,telnet,smtp,http,ftp,port: -- "$@")" || { warn_getopt firewall; return; }
	while :; do
		case $1 in
			--high|--medium|--enabled|--enable|--dhcp|--ssh|--telnet|--smtp|--http|--ftp)
				# TODO: Anaconda uses lokkit; what should we
				# do?
				warn "firewall preseeding not supported yet"
				shift
				;;
			--trust|--port)
				# TODO: Anaconda uses lokkit; what should we
				# do?
				warn "firewall preseeding not supported yet"
				shift 2
				;;
			--disabled|--disable)
				# do nothing
				shift
				;;
			--)	shift; break ;;
			*)	warn_getopt firewall; return ;;
		esac
	done
}
