#! /bin/sh

partition_recipe=
partition_leave_free_space=1

partition_handler () {
	if [ -z "$partition_recipe" ]; then
		partition_recipe='Kickstart-supplied partitioning scheme :'
	fi

	recommended=
	size=
	grow=
	maxsize=
	format=1
	asprimary=
	fstype=

	eval set -- "$(getopt -o '' -l recommended,size:,grow,maxsize:,noformat,onpart:,usepart:,ondisk:,ondrive:,asprimary,fstype:,start:,end: -- "$@")" || { warn_getopt partition; return; }
	while :; do
		case $1 in
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
			--noformat)
				format=
				shift
				;;
			--asprimary)
				asprimary=1
				shift
				;;
			--fstype)
				fstype="$2"
				shift 2
				;;
			--onpart|--usepart|--ondisk|--ondrive|--start|--end)
				warn "unsupported restriction '$1'"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt partition; return ;;
		esac
	done

	if [ $# -ne 1 ]; then
		warn "partition command requires a mountpoint"
		return
	fi
	mountpoint="$1"

	if [ "$mountpoint" = swap ]; then
		filesystem=swap
		mountpoint=
	elif [ "$fstype" ]; then
		filesystem="$fstype"
	else
		filesystem=ext3
	fi

	if [ "$filesystem" = swap ] && [ "$recommended" ]; then
		size=96
		priority=512
		maxsize=300%
		partition_leave_free_space=
	else
		if [ -z "$size" ]; then
			warn "partition command requires a size"
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
	partition_recipe="$partition_recipe $size $priority $maxsize $filesystem"

	if [ "$asprimary" ]; then
		partition_recipe="$partition_recipe \$primary{ }"
	fi

	if [ "$filesystem" = swap ]; then
		partition_recipe="$partition_recipe method{ swap }"
	elif [ "$format" ]; then
		partition_recipe="$partition_recipe method{ format }"
	else
		partition_recipe="$partition_recipe method{ keep }"
	fi

	if [ "$format" ]; then
		partition_recipe="$partition_recipe format{ }"
	fi

	if [ "$filesystem" != swap ]; then
		partition_recipe="$partition_recipe use_filesystem{ }"
		partition_recipe="$partition_recipe filesystem{ $filesystem }"
	fi

	if [ "$mountpoint" ]; then
		partition_recipe="$partition_recipe mountpoint{ $mountpoint }"
	fi

	partition_recipe="$partition_recipe ."
}

part_handler () {
	partition_handler "$@"
}

partition_final () {
	if [ "$partition_recipe" ]; then
		if [ "$partition_leave_free_space" ]; then
			# This doesn't quite work; it leaves a dummy
			# partition at the end of the disk. Bug filed on
			# partman-auto.
			bignum=$((1024 * 1024 * 1024))
			partition_recipe="$partition_recipe 0 $bignum $bignum free ."
		fi
		ks_preseed d-i partman-auto/expert_recipe string \
			"$partition_recipe"
		ks_preseed d-i partman/choose_partition string \
			'Finish partitioning and write changes to disk'
		ks_preseed d-i partman/confirm boolean true
	fi
}

register_final partition_final
