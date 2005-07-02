#!/bin/sh

log () {
	logger -t preseed "$@"
}

error () {
	error="$1"
	location="$2"
	db_subst preseed/$error LOCATION "$location"
	db_input critical preseed/$error || true
	db_go || true
	exit 1
}

# Note: Needs a preseed_fetch function or command not provided by this
# file, as well as preseed_relative
preseed_location () {
	local location="$1"
	local checksum="$2"
	
	local tmp=/tmp/debconf-seed
	local logfile=/var/log/debconf-seed
	
	if ! preseed_fetch $location $tmp; then
		error retrieve_error "$location"
	fi
	if [ -n "$checksum" ] && \
	   [ "$(md5sum $tmp | cut -d' ' -f1)" != "$checksum" ]; then
		error retrieve_error "$location"
	fi

	db_set preseed/include ""
	db_set preseed/include_command ""
	UNSEEN=
	db_get preseed/interactive
	if [ "$RET" = true ]; then
		UNSEEN=--unseen
	fi
	if ! debconf-set-selections $UNSEEN $tmp; then
		error load_error "$location"
	fi
		
	if [ -e $logfile ]; then
		cat $tmp >> $logfile
	else
		cp $tmp $logfile
	fi
	rm -f $tmp

	log "successfully loaded preseed file from $location"
	local last_location="$location"
	
	db_get preseed/include
	local include="$RET"
	db_get preseed/include_command
	if [ -n "$RET" ]; then
		include="$include $(eval $RET)" || true # TODO error handling?
	fi
	if db_get preseed/include/checksum; then
		checksum="$RET"
	else
		checksum=""
	fi
	for location in $include; do
		sum="${checksum%% *}"
		checksum="${checksum#$sum }"

		# Support relative paths, just use path of last file.
		if preseed_relative "$location"; then
			# This works for urls too.
			location="$(dirname $last_location)/$location"
		fi
		if [ -n "$location" ]; then
			preseed_location "$location" "$sum"
		fi
	done
}
	
preseed () {
	template="$1"

	db_get $template
	location="$RET"
	if db_get $template/checksum; then
		checksum="$RET"
	else
		checksum=""
	fi
	for loc in $location; do
		sum="${checksum%% *}"
		checksum="${checksum#$sum }"
		
		preseed_location "$loc" "$sum"
	done
}
