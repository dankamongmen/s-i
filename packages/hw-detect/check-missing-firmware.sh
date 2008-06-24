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
		OLDIFS="$IFS"
		IFS="$NL"
		for line in $(cat $LOG); do
			module="${line%% *}"
			if [ -n "$module" ]; then
				modules="$module $modules"
			fi
			file="${line#* }"
			files="$file $files"
		done
		IFS="$OLDIFS"
		rm -f $LOG
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

while read_log && ask_load_firmware; do
	# try to load udebs (or debs)
	if anna media-retriever; then
		# copy any debs to a holding cache,
		# for installation into /target later
		if mountmedia drivers; then
			mkdir -p /var/cache/firmware/
			cp -a /media/*.deb /var/cache/firmware/ || true
			umount /media || true
		fi
	fi

	# also look for loose firmware files on the media
	if mountmedia; then
		for file in $(files); do
			if [ -e "/media/$file" ]; then
				mkdir -p /lib/firmware
				rm -f "/lib/firmware/$file"
				cp -a "/media/$file" /lib/firmware/ || true
			fi
		done
		umount /media || true
	fi

	# remove and reload modules so they see the new firmware
	for module in $(modules); do
		modprobe -r $module || true
		modprobe $module || true
	done
done
