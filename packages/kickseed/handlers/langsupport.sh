#! /bin/sh

langsupport_default=

langsupport_handler () {
	eval set -- "$(getopt -o '' -l default: -- "$@")" || { warn_getopt langsupport; return; }
	while :; do
		case $1 in
			--default)
				langsupport_default="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt langsupport; return ;;
		esac
	done

	if [ $# -eq 0 ]; then
		warn "langsupport command requires at least one language"
		return
	fi

	languages="$(echo "$*" | sed 's/ /, /g')"
	if [ "$langsupport_default" ]; then
		languages="$langsupport_default, $languages"
	fi

	# requires localechooser 0.04.0ubuntu4
	ks_preseed d-i localechooser/supported-locales multiselect "$languages"
}

langsupport_final () {
	# TODO: no support for different installation and installed languages
	if [ "$lang_value" ] && [ "$langsupport_default" ] && \
			[ "$lang_value" != "$langsupport_default" ]; then
		warn "langsupport --default must equal lang"
	fi
}

register_final langsupport_final
