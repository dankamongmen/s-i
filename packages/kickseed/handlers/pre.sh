#! /bin/sh

pre_handler_section () {
	eval set -- "$(getopt -o '' -l interpreter: -- "$@")" || { warn_getopt %pre; return; }
	while :; do
		case $1 in
			--interpreter)
				if [ "$2" != /bin/sh ]; then
					warn "%pre interpreters other than /bin/sh not supported yet"
				fi
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt %pre; return ;;
		esac
	done
}
