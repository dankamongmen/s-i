#!/bin/sh

set -e

in_image () {
	chroot /live/installer /usr/bin/env -i LIVE_INSTALLER_MODE=1 DEBCONF_FRONTEND=$FRONTEND DISPLAY=:0 TERM=$TERM $@
}

# Launching debian-installer
in_image /sbin/debian-installer-startup
in_image /sbin/debian-installer
