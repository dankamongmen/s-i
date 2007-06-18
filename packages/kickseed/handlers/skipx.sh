#! /bin/sh

skipx_handler () {
	# Ubuntu generally avoids showing an X configuration interface
	# already, so just don't start the display manager.
	ks_preseed base-config base-config/start-display-manager boolean false
}
