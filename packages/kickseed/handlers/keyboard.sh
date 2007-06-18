#! /bin/sh

keyboard_handler () {
	# Specific to Ubuntu keymap selector, but harmless elsewhere.
	ks_preseed d-i kbd-chooser/method 'select' \
		'Select from full keyboard list'

	ks_preseed d-i console-keymaps-at/keymap 'select' "$1"
}
