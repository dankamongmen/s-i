
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
    db_fset partman-auto/autopartitioning_failed seen false
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
		    ext2|ext3|xfs|reiserfs|linux-swap|fat16|fat32)
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
    
#     recipes=$(
# 	if [ -x /bin/archdetect ]; then
# 	    archdetect=$(archdetect)
# 	else
# 	    archdetect=unknown/generic
# 	fi
# 	arch=${archdetect%/*}
# 	sub=${archdetect#*/}
# 	for recipe in \
# 	    /lib/partman/recipes/* \
# 	    /lib/partman/recipes-$arch/* \
# 	    /lib/partman/recipes-$arch-$sub/*
# 	do
# 	    [ -f $recipe ] || continue
# 	    echo ${recipe##*/} ${recipe#/lib/partman/recipes} $recipe
# 	done |
# 	sort | {
# 	    oldname=''
# 	    while read name recipe; do
# 		if [ "$name" != "$oldname" ]; then
# 		    echo $recipe
# 		    oldname="$name"
# 		fi
# 	    done
# 	}
#     )

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
    first_recipe=no
    for recipe in $recipedir/*; do
	[ -f "$recipe" ] || continue
	decode_recipe $recipe
	if [ $(min_size) -le $free_size ]; then
	    choices="${choices}${recipe}${TAB}${name}${NL}"
	    if [ no = "$first_recipe" ]; then
		first_recipe="$recipe"
	    fi
	fi
    done
    
    if [ -z "$choices" ]; then
       db_input critical partman-auto/no_recipe || true
       db_go || true # TODO handle backup right
       return 1
    fi
    
#    db_metaget partman-auto/text/expert_recipe description
#    choices="${choices}expert${TAB}${RET}"
    
    db_fset partman-auto/choose_recipe seen false
    debconf_select high partman-auto/choose_recipe "$choices" "$first_recipe"
    if [ "$?" = 255 ]; then
	exit 0
    fi
    
    if [ "$RET" = expert ]; then
	db_fset partman-auto/expert_recipe seen false
	db_input critical partman-auto/expert_recipe || true
	if ! db_go; then
	    exit 1
	fi
    fi
    recipe="$RET"
}
