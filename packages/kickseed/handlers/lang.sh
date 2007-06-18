#! /bin/sh

# used in langsupport consistency check
lang_value=

lang_handler () {
	lang_value="$1"
	# for localechooser << 0.07; harmless otherwise
	ks_preseed d-i preseed/locale string "$1"
	# for localechooser >= 0.07; harmless otherwise
	ks_preseed d-i debian-installer/locale string "$1"
}
