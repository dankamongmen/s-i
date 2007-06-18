#! /bin/sh

lang_handler () {
	preseed d-i preseed/locale string "$1"
}
