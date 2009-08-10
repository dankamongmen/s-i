#! /bin/sh

repo_next_local_id=0

repo_handler () {
	name=
	baseurl=
	distribution=
	components=
	enable_source=
	key=

	# TODO --mirrorlist=
	eval set -- "$(getopt -o '' -l name:,baseurl:,distribution:,components:,source,key: -- "$@")" || { warn_getopt repo; return; }
	while :; do
		case $1 in
			--name)
				name="$2"
				shift 2
				;;
			--baseurl)
				baseurl="$2"
				shift 2
				;;
			--distribution)
				# Debian extension
				distribution="$2"
				shift 2
				;;
			--components)
				# Debian extension
				components="$(echo "$2" | sed 's/,/ /g')"
				shift 2
				;;
			--source)
				# Debian extension
				enable_source=1
				shift
				;;
			--key)
				# Debian extension
				key="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt repo; return ;;
		esac
	done

	if [ -z "$baseurl" ]; then
		warn "repo command requires --baseurl"
		return
	fi

	if [ "$components" ] && [ -z "$distribution" ]; then
		warn "repo --components meaningless without --distribution; ignoring --components"
	fi

	prefix="apt-setup/local$repo_next_local_id"
	if [ "$distribution" ]; then
		ks_preseed d-i "$prefix/repository" string "$baseurl $distribution${components:+ $components}"
	else
		ks_preseed d-i "$prefix/repository" string "$baseurl ./"
	fi
	if [ "$name" ]; then
		ks_preseed d-i "$prefix/comment" string "$name"
	fi
	if [ "$enable_source" ]; then
		ks_preseed d-i "$prefix/source" boolean true
	fi
	if [ "$key" ]; then
		ks_preseed d-i "$prefix/key" string "$key"
	fi
	repo_next_local_id="$(($repo_next_local_id + 1))"
}
