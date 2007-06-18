#! /bin/sh

if [ -z "$SPOOL" ]; then
	SPOOL=/var/spool/kickseed
fi

# Find the parameter named by $2 in /proc/cmdline (or the test file of your choice).
kickseed_cmdline () {
	for item in $(cat "$1"); do
		var="${item%%=*}"

		if [ "$var" = "$2" ]; then
			echo "${item#*=}"
			break
		fi
	done
}

# Work out the filename corresponding to a ks parameter.
kickseed_file () {
	if [ -z "$1" ]; then
		return
	fi
	case $1 in
		cdrom:/*)
			echo "/cdrom${1#cdrom:}"
			;;
		file:/*)
			echo "${1#file:}"
			;;
		floppy)
			echo /floppy/ks.cfg
			;;
		floppy:/*)
			echo "/floppy${1#floppy:}"
			;;
		ftp://*/*)
			spoolpath="$SPOOL/fetch/ftp/${1#ftp://}"
			mkdir -p "${spoolpath%/*}"
			echo "$spoolpath"
			;;
		hd:*:/*)
			file="${1#hd:}"
			device="${file%%:*}"
			file="${file#*:}"
			echo "/media/$device$file"
			;;
		http://*/*)
			spoolpath="$SPOOL/fetch/http/${1#http://}"
			mkdir -p "${spoolpath%/*}"
			echo "$spoolpath"
			;;
		nfs:*:/*)
			file="${1#nfs:}"
			server="${file%%:*}"
			file="${file#*:}"
			spoolpath="$SPOOL/fetch/nfs/$server/$file"
			mkdir -p "${spoolpath%/*}"
			echo "$spoolpath"
			;;
		*)
			logger -t kickseed "Kickstart location $1 not supported"
			;;
	esac
}
