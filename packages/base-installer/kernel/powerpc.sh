arch_get_kernel_flavour () {
	CPU=`grep '^cpu[[:space:]]*:' /proc/cpuinfo | head -n1 | cut -d: -f2 | sed 's/^ *//; s/[, ].*//' | tr A-Z a-z`
	case "$CPU" in
		power3|i-star|s-star)
			family=power3
			;;
		power4|power4+|ppc970|ppc970fx)
			family=power4
			;;
		*)
			family=powerpc
			;;
	esac
	case "$SUBARCH" in
		powermac_newworld)	echo "$family" ;;
		powermac_oldworld)	echo "$family" ;;
		prep)			echo "$family" ;;
		chrp*)			echo "$family" ;;
		amiga)			echo apus ;;
		*)
			warning "Unknown $ARCH subarchitecture '$SUBARCH'."
			return 1
		;;
	esac
	return 0
}

arch_check_usable_kernel () {
	# CPU family and subarchitecture must match exactly.
	family="${2%%-*}"
	if ! expr "$1" : ".*-$family.*" >/dev/null; then return 1; fi
	# 2.6 kernels don't differ by subarchitecture.
	if expr "$1" : ".*-2\.6.*" >/dev/null; then return 0; fi
	subarch="${2#*-}"
	if expr "$1" : ".*-$subarch.*" >/dev/null; then return 0; fi
	return 1
}

arch_get_kernel () {
	version=
	case "$KERNEL_MAJOR" in
		2.6)	version=2.6.8 ;;
		*)	version=2.4.27 ;;
	esac
	# The APUS kernels are in a separate source package, so may
	# sometimes have a different version number.
	apusversion=2.4.27

	CPUS="$(grep -ci ^processor /proc/cpuinfo)"
	if [ "$CPUS" ] && [ "$CPUS" -gt 1 ]; then
		SMP=-smp
	else
		SMP=
	fi

	family="${1%%-*}"
	subarch="${1#*-}"

	case "$1" in
		apus)	trykernel=kernel-image-$apusversion-apus ;;
		*)
			case "$KERNEL_MAJOR" in
				2.6)
					echo "kernel-image-$version-$family$SMP"
					;;
				*)
					if [ "$family" != powerpc ]; then
						# 2.4 only has powerpc-smp.
						SMP=
					fi
					echo "kernel-image-$version-$family$SMP-$subarch"
					;;
			esac
	esac
}
