make_absolute_url() {
	url=$1

	if [ -n "${url##*://*}" ]; then
		if [ -z "${url##/*}" ]; then
			if [ -z "${2##*/./*}" ]; then
				url=${2%%/./*}/.$url
			else
				url="$(expr $x : '\([^:]*://[^/]*/\)')$url"
			fi
		else
			url=${2%/*}/$url
		fi
	fi
	echo "$url"
}
