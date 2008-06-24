#!/bin/sh
set -e
. /usr/share/debconf/confmodule

LOG=/tmp/missing-firmware
NL="
"

read_log () {
	modules=""
	files=""
	if [ -s "$LOG" ]; then
		mv $LOG $LOG.old
		OLDIFS="$IFS"
		IFS="$NL"
		for line in $(cat $LOG.old); do
			module="${line%% *}"
			if [ -n "$module" ]; then
				modules="$module $modules"
			fi
			file="${line#* }"
			if [ -n "$file" ]; then
				files="$file $files"
			fi
		done
		IFS="$OLDIFS"
		rm -f $LOG.old
	fi

	if [ -n "$modules" ]; then
		return 0
	else
		return 1
	fi
}

ask_load_firmware () {
	db_subst hw-detect/load_firmware FILES "$files"
	db_input high hw-detect/load_firmware || true
	db_go || true
	db_get hw-detect/load_firmware
	if [ "$RET" = true ]; then
		return 0
	else
		return 1
	fi
}

listdeb_firmware () {
	ar p "$1" data.tar.gz | tar zt \
		| grep '^\./lib/firmware/' \
		| sed -e 's!^\./lib/firmware/!!'
}

install_firmware_pkg () {
	if echo "$1" | grep -q '\.deb$'; then
		# cache deb for installation into /target later
		mkdir -p /var/cache/firmware/
		cp -a "$1" /var/cache/firmware/ || true
		udpkg --unpack "/var/cache/firmware/$(basename "$1")"
	else
		udpkg --unpack "$1"
	fi
}

while read_log && ask_load_firmware; do
	# first, look for loose firmware files on the media.
	if mountmedia; then
		for file in $files; do
			if [ -e "/media/$file" ]; then
				mkdir -p /lib/firmware
				rm -f "/lib/firmware/$file"
				cp -a "/media/$file" /lib/firmware/ || true
			fi
		done
		umount /media || true
	fi

	# Try to load udebs (or debs) that contain the missing firmware.
	# This does not use anna because debs can have arbitrary
	# dependencies, which anna might try to install.
	if mountmedia drivers; then
		grepfor="$(echo "$files" | sed -e 's/ /\n/')"
		for filename in /media/*.deb /media/*.udeb /media/*.ude; do
			if [ -f "$filenmame" ]; then
				if list_deb_firmware "$filename" | grep -qF "$grepfor"; then
					unstall_firmware_pkg "$filename" || true
				fi
			fi
		done
		umount /media || true
	fi

	# remove and reload modules so they see the new firmware
	for module in $modules; do
		modprobe -r $module || true
		modprobe $module || true
	done
done
