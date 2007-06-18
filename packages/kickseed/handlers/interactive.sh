#! /bin/sh

interactive_handler () {
	# requires di-utils 1.09, preseed-common 1.03
	ks_preseed d-i preseed/interactive boolean true
}
