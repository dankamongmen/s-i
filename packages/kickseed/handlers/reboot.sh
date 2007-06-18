#! /bin/sh

reboot_handler () {
	ks_preseed d-i prebaseconfig/reboot_in_progress note ''
	ks_preseed d-i finish-install/reboot_in_progress note ''
}
