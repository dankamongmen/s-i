#! /bin/sh

keyboard_handler () {
	preseed d-i console-keymaps-at/keymap 'select' "$1"
}
