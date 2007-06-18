#! /bin/sh

firewall_handler () {
	eval set -- "$(getopt -o '' -l high,medium,enabled,enable,disabled,disable,trust:,dhcp,ssh,telnet,smtp,http,ftp,port: -- "$@")" || die_getopt firewall
	while :; do
		case $1 in
			--high|--medium|--enabled|--enable|--trust|--dhcp|--ssh|--telnet|--smtp|--http|--ftp|--port)
				# TODO: Anaconda uses lokkit; what should we
				# do?
				die "firewall preseeding not supported yet"
				;;
			--disabled|--disable)
				# do nothing
				shift
				;;
			--)	shift; break ;;
			*)	die_getopt firewall
		esac
	done
}
