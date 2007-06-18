#! /bin/sh

reboot_handler () {
	preseed d-i prebaseconfig/reboot_in_progress note ''
}
