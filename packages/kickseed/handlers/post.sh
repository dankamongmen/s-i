#! /bin/sh

post_handler_section () {
	# Global variables read by kickseed.
	post_chroot=1
	post_interpreter=

	eval set -- "$(getopt -o '' -l nochroot,interpreter: -- "$@")" || { warn_getopt %post; return; }
	while :; do
		case $1 in
			--nochroot)
				post_chroot=0
				shift
				;;
			--interpreter)
				post_interpreter="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt %post; return ;;
		esac
	done

	if [ "$post_chroot" = 0 ] && [ "$post_interpreter" ] && \
	   [ "$post_interpreter" != /bin/sh ]; then
		warn "%post --nochroot interpreters other than /bin/sh not supported yet"
	fi
}
