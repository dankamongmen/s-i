
# If you are curious why partman-auto is so slow, it is because
# update-all is slow
update_all () {
    local dev num id size type fs path name partitions
    for dev in $DEVICES/*; do
	[ -d "$dev" ] || continue
	cd $dev
	partitions=''
	open_dialog PARTITIONS
	while { read_line num id size type fs path name; [ "$id" ]; }; do
	    partitions="$partitions $id"
	done
	close_dialog
	for id in $partitions; do
	    update_partition $dev $id
	done
    done
}

autopartitioning_failed () {
    db_input critical partman-auto/autopartitioning_failed || true
    db_go || true
    update_all
    exit 1
}

unnamed=0

decode_recipe () {
    local ram line word min factor max fs -
    unnamed=$(($unnamed + 1))
    ram=$(grep ^Mem: /proc/meminfo | { read x y z; echo $y; }) # in bytes
    if [ -z "$ram" ]; then
    	ram=$(grep ^MemTotal: /proc/meminfo | { read x y z; echo $y; })000
    fi
    ram=$(expr 0000000"$ram" : '0*\(..*\)......$') # convert to megabytes
    name="Unnamed.${unnamed}"
    scheme=''
    line=''
    for word in $(cat $1); do
	case $word in
	    :)
		name=$line
		line=''
		;;
	    ::)
		db_metaget $line description
		if [ "$RET" ]; then
		    name=$RET
		else
		    name="Unnamed.${unnamed}"
		fi
		line=''
		;;
	    .)
		# we correct errors in order not to crash parted_server
		set -- $line
		if expr "$1" : '[0-9][0-9]*$' >/dev/null; then
		    min=$1
		elif expr "$1" : '[0-9][0-9]*%$' >/dev/null; then
		    min=$(($ram * ${1%?} / 100))
		else # error
		    min=2200000000 # there is no so big storage device jet
		fi
		if expr "$2" : '[0-9][0-9]*%$' >/dev/null; then
		    factor=$(($ram * ${2%?} / 100))
		elif expr "$2" : '[0-9][0-9]*$' >/dev/null; then
		    factor=$2
		else # error
		    factor=$min # do not enlarge the partition
		fi
		if [ "$factor" -lt "$min" ]; then
		    factor="$min"
		fi
		if expr "$3" : '[0-9][0-9]*$' >/dev/null; then
		    max=$3
		elif expr "$3" : '[0-9][0-9]*%$' >/dev/null; then
		    max=$(($ram * ${3%?} / 100))
		else # error
		    max=$min # do not enlarge the partition
		fi
		if [ "$max" -lt "$min" ]; then
		    max="$min"
		fi
		case "$4" in # allow only valid file systems
		    ext2|ext3|xfs|reiserfs|jfs|linux-swap|fat16|fat32|hfs)
			fs="$4"
			;;
		    *)
			fs=ext2
			;;
		esac
		shift; shift; shift; shift
		line="$min $factor $max $fs $*"
		if [ "$scheme" ]; then
		    scheme="${scheme}${NL}${line}"
		else
		    scheme="$line"
		fi
		line=''
		;;
	    *)
		if [ "$line" ]; then
		    line="$line $word"
		else
		    line="$word"
		fi
	esac
    done
}

foreach_partition () {
    local - doing IFS partition former last
    doing=$1
    IFS="$NL"
    former=''
    for partition in $scheme; do
	restore_ifs
	if [ "$former" ]; then
	    set -- $former
	    last=no
	    eval "$doing"
	fi
	former="$partition"
    done
    if [ "$former" ]; then
	set -- $former
	last=yes
	eval "$doing"
    fi
}

min_size () {
    local size
    size=0
    foreach_partition '
	size=$(($size + $1))'
    echo $size
}

factor_sum () {
    local factor
    factor=0
    foreach_partition '
	factor=$(($factor + $2))'
    echo $factor
}

partition_before () {
    local num id size type fs path name result found
    result=''
    found=no
    open_dialog PARTITIONS
    while { read_line num id size type fs path name; [ "$id" ]; }; do
	if [ "$id" = "$1" ]; then
	    found=yes
	fi
	if [ $found = no ]; then
	    result=$id
	fi
    done
    close_dialog
    echo $result
}

partition_after () {
    local num id size type fs path name result found
    result=''
    found=no
    open_dialog PARTITIONS
    while { read_line num id size type fs path name; [ "$id" ]; }; do
	if [ $found = yes -a -z "$result" ]; then
	    result=$id
	fi
	if [ "$id" = "$1" ]; then
	    found=yes
	fi
    done
    close_dialog
    echo $result
}

pull_primary () {
    primary=''
    logical=''
    foreach_partition '
        if
            [ -z "$primary" ] \
            && echo $* | grep '\''\$primary{'\'' >/dev/null
        then
            primary="$*"
        else
    	    if [ -z "$logical" ]; then
    	        logical="$*"
    	    else
                logical="${logical}${NL}$*"
    	    fi
        fi'
}

setup_partition () {
    local id flags file line
    id=$1; shift
    while [ "$1" ]; do
	case "$1" in
	    \$bootable{)
	        while [ "$1" != '}' -a "$1" ]; do
		    shift
		done
		open_dialog GET_FLAGS $id
		flags=$(read_paragraph)
		close_dialog
		open_dialog SET_FLAGS $id
		write_line "$flags"
		write_line boot
		write_line NO_MORE
		close_dialog
		;;
	    \$*{)
                while [ "$1" != '}' -a "$1" ]; do
		    shift
		done
		;;
	    *{)
		file=${1%?}
		[ -d $id ] || mkdir $id
		case $file in
		    */*)
			mkdir -p $id/${file%/*}
			;;
		esac
		>$id/$file
		shift
		line=''
	        while [ "$1" != '}' -a "$1" ]; do
		    if [ "$1" = ';' ]; then
			echo "$line" >>$id/$file
		    else
			if [ "$line" ]; then
			    line="$line $1"
			else
			    line="$1"
			fi
		    fi
		    shift
		done
		echo "$line" >>$id/$file
	esac
	shift
    done
    return 0
}

choose_recipe () {
    local recipes archdetect arch sub free_size choices min_size
    
    # Preseeding of recipes.
    db_get partman-auto/expert_recipe
    if [ -n "$RET" ]; then
	echo "$RET" > /tmp/expert_recipe
	db_set partman-auto/expert_recipe_file /tmp/expert_recipe
    fi
    db_get partman-auto/expert_recipe_file
    if [ ! -z "$RET" ] && [ -e "$RET" ]; then
        recipe="$RET"
	decode_recipe $recipe
	if [ $(min_size) -le $free_size ]; then
	    return 0
	fi
    fi

    if [ -x /bin/archdetect ]; then
	archdetect=$(archdetect)
    else
	archdetect=unknown/generic
    fi
    arch=${archdetect%/*}
    sub=${archdetect#*/}

    for recipedir in \
	/lib/partman/recipes-$arch-$sub \
	/lib/partman/recipes-$arch \
	/lib/partman/recipes
    do
        if [ -d $recipedir ]; then
	    break
	fi
    done

    
    free_size=$1
    choices=''
    default_recipe=no
    db_get partman-auto/choose_recipe
    old_default_recipe="$RET"
    for recipe in $recipedir/*; do
	[ -f "$recipe" ] || continue
	decode_recipe $recipe
	if [ $(min_size) -le $free_size ]; then
	    choices="${choices}${recipe}${TAB}${name}${NL}"
	    if [ no = "$default_recipe" ]; then
		default_recipe="$recipe"
	    fi
	    if [ "$old_default_recipe" = "$name" ]; then
		default_recipe="$recipe"
            fi
	fi
    done
    
    if [ -z "$choices" ]; then
       db_input critical partman-auto/no_recipe || true
       db_go || true # TODO handle backup right
       return 1
    fi
 
    debconf_select high partman-auto/choose_recipe "$choices" "$default_recipe"
    if [ "$?" = 255 ]; then
	exit 0
    fi
    recipe="$RET"
}
