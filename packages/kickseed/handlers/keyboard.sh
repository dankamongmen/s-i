#! /bin/sh

keyboard_handler () {
	# Specific to Ubuntu keymap selector, but harmless elsewhere.
	ks_preseed d-i kbd-chooser/method 'select' \
		'Select from full keyboard list'

	for kbarch in acorn amiga at atari dec mac sun usb; do
		ks_preseed d-i "console-keymaps-$kbarch/keymap" 'select' "$1"
	done

	# requires console-setup; keymap names often incompatible with old
	# kbd-chooser preseeding
	ks_preseed d-i console-setup/layoutcode string "${1%%_*}"
	if [ "${1#*_}" != "$1" ]; then
		ks_preseed d-i console-setup/variantcode string "${1#*_}"
	fi
}
