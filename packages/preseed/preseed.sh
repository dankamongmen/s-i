#!/bin/sh

. /lib/preseed/functions.sh

logfile=/var/lib/preseed/log

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

preseed_location () {
	local location="$1"
	local checksum="$2"
	
	local tmp=/tmp/debconf-seed
	
	if ! preseed_fetch "$location" "$tmp"; then
		error retrieve_error "$location"
	fi
	if [ -n "$checksum" ] && \
	   [ "$(md5sum $tmp | cut -d' ' -f1)" != "$checksum" ]; then
		error retrieve_error "$location"
	fi

	db_set preseed/include ""
	db_set preseed/include_command ""
	db_set preseed/run ""
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
	
	while true ; do
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
		db_get preseed/run
		local torun="$RET"

		# not really sure if the ones above are required if this is here
		db_set preseed/include ""
		db_set preseed/include_command ""
		db_set preseed/run ""

		[ -n "$include" -o -n "$torun" ] || break

		for location in $include; do
			sum="${checksum%% *}"
			checksum="${checksum#$sum }"

			location=$(make_absolute_url "$location" "$last_location")
			# BTW -- is this test for empty strings really needed?
			if [ -n "$location" ]; then
				preseed_location "$location" "$sum"
			fi
		done
	
		db_set preseed/last_location $last_location
		for location in $torun; do
			location=$(make_absolute_url "$location" "$last_location")
			# BTW -- is this test for empty strings really needed?
			if [ -n "$location" ]; then
				if ! preseed_fetch "$location" "$tmp"; then
					error retrieve_error "$location"
				fi
				chmod +x $tmp
				if ! $tmp; then
					error load_error "$location"
				fi
				log "successfully ran file from $location"
				rm -f $tmp
			fi
		done
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
