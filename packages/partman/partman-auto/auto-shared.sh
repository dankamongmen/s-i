## this file contains a bunch of shared code between partman-auto
## and partman-auto-lvm.

wipe_disk() {
	cd $dev

	open_dialog LABEL_TYPES
	types=$(read_list)
	close_dialog

	label_type=$(default_disk_label)

	if ! expr "$types" : ".*${label_type}.*" >/dev/null; then
		label_type=msdos # most widely used
	fi
	
	# Use gpt instead of msdos disklabel for disks larger than 2TB
	if expr "$types" : ".*gpt.*" >/dev/null; then
		if [ "$label_type" = msdos ] ; then
			disksize=$(cat size)
			if $(longint_le $(human2longint 2TB) $disksize) ; then
				label_type=gpt
			fi
		fi
	fi

	if [ "$label_type" = sun ]; then
		db_input critical partman/confirm_write_new_label
		db_go || exit 0
		db_get partman/confirm_write_new_label
		if [ "$RET" = false ]; then
			db_reset partman/confirm_write_new_label
			exit 1
		fi
		db_reset partman/confirm_write_new_label
	fi
	
	open_dialog NEW_LABEL "$label_type"
	close_dialog
	
	if [ "$label_type" = sun ]; then
		# write the partition table to the disk
		disable_swap
		open_dialog COMMIT
		close_dialog
		sync
		# reread it from there
		open_dialog UNDO
		close_dialog
		enable_swap
	fi

	# Different types partition tables support different visuals.  Some
	# have partition names other don't have, some have extended and
	# logical partitions, others don't.  Hence we have to regenerate the
	# list of the visuals
	rm -f visuals

	free_space=''
	open_dialog PARTITIONS
	while { read_line num id size type fs path name; [ "$id" ]; }; do
		if [ "$fs" = free ]; then
			free_space=$id
			free_size=$size
			free_type=$type
		fi
	done
	close_dialog
}

### XXXX: i am not 100% sure if this is exactly what this code is doing.
### XXXX: rename is of course an option. Just remember to do it here, in
### XXXX: perform_recipe and in partman-auto-lvm
create_primary_partitions() {

	cd $dev

	while
	    [ "$free_type" = pri/log ] \
	    && echo $scheme | grep '\$primary{' >/dev/null
	do
	    pull_primary
	    set -- $primary
	    open_dialog NEW_PARTITION primary $4 $free_space beginning ${1}000001
	    read_line num id size type fs path name
	    close_dialog
	    if [ -z "$id" ]; then
		db_progress STOP
		autopartitioning_failed
	    fi
	    neighbour=$(partition_after $id)
	    if [ "$neighbour" ]; then
		open_dialog PARTITION_INFO $neighbour
		read_line x1 new_free_space x2 new_free_type fs x3 x4
		close_dialog
	    fi
	    if 
		[ -z "$neighbour" -o "$fs" != free \
		  -o "$new_free_type" = primary -o "$new_free_type" = unusable ]
	    then
		open_dialog DELETE_PARTITION $id
		close_dialog
		open_dialog NEW_PARTITION primary $4 $free_space end ${1}000001
		read_line num id size type fs path name
		close_dialog
		if [ -z "$id" ]; then
		    db_progress STOP
		    autopartitioning_failed
		fi
		neighbour=$(partition_before $id)
		if [ "$neighbour" ]; then
		    open_dialog PARTITION_INFO $neighbour
		    read_line x1 new_free_space x2 new_free_type fs x3 x4
		    close_dialog
		fi
		if 
		    [ -z "$neighbour" -o "$fs" != free -o "$new_free_type" = unusable ]
		then
		    open_dialog DELETE_PARTITION $id
		    close_dialog
		    break
		fi
	    fi
	    shift; shift; shift; shift
	    setup_partition $id $*
	    primary=''
	    scheme="$logical"
	    free_space=$new_free_space
	    free_type="$new_free_type"
	done
}

create_partitions() {
foreach_partition '
    if [ -z "$free_space" ]; then
        db_progress STOP
	autopartitioning_failed
    fi
    open_dialog PARTITION_INFO $free_space
    read_line x1 free_space x2 free_type fs x3 x4
    close_dialog
    if [ "$fs" != free ]; then
        free_type=unusable
    fi
    case "$free_type" in
	primary|logical)
	    type="$free_type"
	    ;;
	pri/log)
	    type=logical
	    ;;
	unusable)
            db_progress STOP
	    autopartitioning_failed
	    ;;
    esac
    if [ "$last" = yes ]; then
        open_dialog NEW_PARTITION $type $4 $free_space full ${1}000001
    else
        open_dialog NEW_PARTITION $type $4 $free_space beginning ${1}000001
    fi
    read_line num id size type fs path name
    close_dialog
    if [ -z "$id" ]; then
        db_progress STOP
	autopartitioning_failed
    fi
    # Mark the partition LVM only if it is actually LVM and add it to vgpath
    if echo "$*" | grep -q "method{ lvm }"; then
	devfspv_devices="$devfspv_devices $path"
	open_dialog GET_FLAGS $id
	flags=$(read_paragraph)
	close_dialog
	open_dialog SET_FLAGS $id
	write_line "$flags"
	write_line lvm
	write_line NO_MORE
	close_dialog
    fi
    shift; shift; shift; shift
    setup_partition $id $*
    free_space=$(partition_after $id)'
}
