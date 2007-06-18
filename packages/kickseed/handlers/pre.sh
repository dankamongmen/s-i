#! /bin/sh

pre_handler_section () {
	eval set -- "$(getopt -o '' -l interpreter -- "$@")" || die_getopt %pre
	while :; do
		case $1 in
			--interpreter)
				if [ "$2" != /bin/sh ]; then
					die "%pre interpreters other than /bin/sh not supported yet"
				fi
				shift 2
				;;
			--)	shift; break ;;
			*)	die_getopt %pre ;;
		esac
	done
}
