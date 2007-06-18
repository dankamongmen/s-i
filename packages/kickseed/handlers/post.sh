#! /bin/sh

post_chroot=1

post_handler_section () {
	eval set -- "$(getopt -o '' -l nochroot,interpreter: -- "$@")" || die_getopt %post
	while :; do
		case $1 in
			--nochroot)
				post_chroot=0
				shift
				;;
			--interpreter)
				if [ "$2" != /bin/sh ]; then
					die "%post interpreters other than /bin/sh not supported yet"
				fi
				shift 2
				;;
			--)	shift; break ;;
			*)	die_getopt %post ;;
		esac
	done
}
