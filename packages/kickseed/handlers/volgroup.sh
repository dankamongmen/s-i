#! /bin/sh

volgroup_pull_recipe () {
	vgname="$1"
	want_pvname="$2"
	shift 2
	got_pvname=
	line=
	rest=
	while [ "$1" ]; do
		case $1 in
			\$pending_lvm{)
				shift
				got_pvname="$1"
				shift
				;;
			.)
				if [ "$want_pvname" = "$got_pvname" ]; then
					partition_recipe_append "$line vg_name{ $vgname }"
				else
					rest="${rest:+$rest }$line ."
				fi
				line=
				;;
			*)
				line="${line:+$line }$1"
				;;
		esac
		shift
	done
	partition_pending_lvm_recipe="$rest"
}

volgroup_handler () {
	existing=

	eval set -- "$(getopt -o '' -l noformat,useexisting,pesize: -- "$@")" || { warn_getopt volgroup; return; }
	while :; do
		case $1 in
			--noformat|--useexisting)
				existing=1
				# TODO?
				;;
			--pesize)
				warn "only default PE size supported"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt volgroup; return ;;
		esac
	done

	if [ $# -ne 2 ]; then
		warn "volgroup command requires volume group name and partition"
		return
	fi

	volgroup_pull_recipe "$1" "$2" $partition_pending_lvm_recipe
}
