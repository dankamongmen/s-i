#! /bin/sh

# used in langsupport consistency check
lang_value=

lang_handler () {
	lang_value="$1"
	ks_preseed d-i preseed/locale string "$1"
}
