#!/bin/sh

set -e

in_image () {
	chroot /live/installer /usr/bin/env -i LIVE_INSTALLER_MODE=1 DEBIAN_FRONTEND=$DI_FRONTEND DISPLAY=:0 TERM=$TERM $@
}

# Launching debian-installer
in_image /sbin/debian-installer-startup
in_image /sbin/debian-installer
