#! /bin/sh

interactive_handler () {
	# requires debian-installer-utils 1.09, preseed 1.03
	preseed d-i preseed/interactive boolean true
}
