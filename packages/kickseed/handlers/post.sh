#! /bin/sh

post_chroot=1

post_handler_section () {
	eval set -- "$(getopt -o '' -l nochroot,interpreter: -- "$@")" || { warn_getopt %post; return; }
	while :; do
		case $1 in
			--nochroot)
				post_chroot=0
				shift
				;;
			--interpreter)
				if [ "$2" != /bin/sh ]; then
					warn "%post interpreters other than /bin/sh not supported yet"
				fi
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt %post; return ;;
		esac
	done
}
