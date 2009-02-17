#! /bin/sh

logvol_handler () {
	vgname=
	recommended=
	size=
	grow=
	maxsize=
	name=
	format=1
	fstype=

	# TODO --percent=, --bytes-per-inode=, --fsoptions=
	eval set -- "$(getopt -o '' -l vgname:,recommended,size:,grow,maxsize:,name:,noformat,fstype: -- "$@")" || { warn_getopt logvol; return; }
	while :; do
		case $1 in
			--vgname)
				vgname="$2"
				shift 2
				;;
			--recommended)
				# Apparently only used for swap, but
				# Anaconda doesn't error if you try to use
				# it for something else, so we won't either.
				recommended=1
				shift
				;;
			--size)
				size="$2"
				shift 2
				;;
			--grow)
				grow=1
				shift
				;;
			--maxsize)
				maxsize="$2"
				shift 2
				;;
			--name)
				name="$2"
				shift 2
				;;
			--noformat)
				format=
				shift
				;;
			--fstype)
				fstype="$2"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt logvol; return ;;
		esac
	done

	if [ $# -ne 1 ]; then
		warn "logvol command requires a mountpoint"
		return
	fi
	mountpoint="$1"

	if [ -z "$vgname" ]; then
		warn "logvol command requires a vgname"
		return
	fi
	# TODO check volume group exists?
	if [ -z "$name" ]; then
		warn "logvol command requires a name"
		return
	fi

	case $mountpoint in
		swap)
			filesystem=linux-swap
			mountpoint=
			;;
		*)
			if [ "$fstype" ]; then
				filesystem="$fstype"
			else
				filesystem=ext3
			fi
			;;
	esac

	# TODO: clone-and-hack from partition_handler
	if [ "$filesystem" = linux-swap ] && [ "$recommended" ]; then
		size=96
		priority=512
		maxsize=300%
		partition_leave_free_space=
	else
		if [ -z "$size" ]; then
			warn "logvol command requires a size"
			return
		fi
		if [ "$grow" ]; then
			partition_leave_free_space=
			if [ -z "$maxsize" ]; then
				maxsize=$((1024 * 1024 * 1024))
			fi
			priority="$maxsize"
		else
			maxsize="$size"
			priority="$size"
		fi
	fi
	new_recipe="$size $priority $maxsize $filesystem"
	new_recipe="$new_recipe \$lvmok{ } in_vg{ $vgname } lv_name{ $name }"

	if [ "$filesystem" = linux-swap ]; then
		new_recipe="$new_recipe method{ swap }"
	elif [ "$format" ]; then
		new_recipe="$new_recipe method{ format }"
	else
		new_recipe="$new_recipe method{ keep }"
	fi

	if [ "$format" ]; then
		new_recipe="$new_recipe format{ }"
	fi

	if [ "$filesystem" != linux-swap ]; then
		new_recipe="$new_recipe use_filesystem{ }"
		new_recipe="$new_recipe filesystem{ $filesystem }"
	fi

	if [ "$mountpoint" ]; then
		new_recipe="$new_recipe mountpoint{ $mountpoint }"
	fi

	partition_recipe_append "$new_recipe"

	[ "$clearpart_method" ] || clearpart_method=lvm
}

logvol_final () {
	ks_preseed d-i partman-lvm/confirm boolean true
}

register_final logvol_final
