#! /bin/sh

keyboard_handler () {
	ks_preseed d-i console-keymaps-at/keymap 'select' "$1"
}
