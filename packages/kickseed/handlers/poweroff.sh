#! /bin/sh

poweroff_handler () {
	reboot_handler
	ks_preseed d-i debian-installer/exit/poweroff boolean true
}
