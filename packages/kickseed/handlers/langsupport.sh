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
		# Support all locales (per the Red Hat Kickstart
		# documentation). Yes, this will be slow.
		languages=
		for l in $(grep -v '\.UTF-8@euro$' "$SUPPORTEDLOCALES"); do
			languages="${languages:+$languages, }$l"
		done
		# Per Anaconda, default to en_US.UTF-8 if there's no default
		# set.
		if [ -z "$langsupport_default" ]; then
			langsupport_default=en_US.UTF-8
		fi
	else
		languages="$(echo "$*" | sed 's/ /, /g')"

		# If there's a default, support that too; otherwise, the
		# first locale selected is the default.
		# (According to the Red Hat Kickstart documentation, it's an
		# error not to specify a default when asking for support for
		# more than one locale; however, Anaconda just picks the
		# first one, so we'll go with that.)
		if [ "$langsupport_default" ]; then
			languages="$langsupport_default, $languages"
		else
			langsupport_default="$1"
		fi
	fi

	# requires localechooser 0.06
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
