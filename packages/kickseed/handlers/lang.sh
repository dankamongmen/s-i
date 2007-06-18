#! /bin/sh

lang_handler () {
	ks_preseed d-i preseed/locale string "$1"
}
