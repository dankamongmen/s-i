#! /bin/sh

halt_handler () {
	reboot_handler
	ks_preseed d-i debian-installer/exit/halt boolean true
}
