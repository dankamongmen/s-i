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
	
	local tmp=/tmp/debconf-seed
	local logfile=/var/log/debconf-seed
	
	if ! preseed_fetch $location $tmp; then
		error retrieve_error "$location"
	fi

	db_set preseed/include ""
	db_set preseed/include_command ""
	if ! debconf-set-selections $tmp; then
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
	
	for location in $include; do
		# Support relative paths, just use path of last file.
		if preseed_relative "$location"; then
			# This works for urls too.
			location="$(dirname $last_location)/$location"
		fi
		if [ -n "$location" ]; then
			preseed_location "$location"
		fi
	done
}
	
preseed () {
	template="$1"

	db_get $template
	location="$RET"
	if [ -n "$location" ]; then
		for loc in $location; do
			preseed_location "$loc"
		done
	fi
}
