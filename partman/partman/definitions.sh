
. /usr/share/debconf/confmodule

TAB='	'
NL='
'
ORIGINAL_IFS="${ORIGINAL_IFS:-$IFS}"; export ORIGINAL_IFS

restore_ifs () {
	IFS="$ORIGINAL_IFS"
}

dirname () {
	local x
	x="${1%/}"
	echo "${x%/*}"
}

basename () {
	local x
	x="${1%$2}"
	x="${x%/}"
	echo "${x##*/}"
}

debconf_select () {
	local IFS priority template choices default_choice default x u newchoices code
	priority="$1"
	template="$2"
	choices="$3"
	default_choice="$4"
	default=''
	# Debconf ignores spaces so we have to remove them from
	# $choices
	newchoices=''
	IFS="$NL"
	for x in $choices; do
		local key option
		restore_ifs
		key=$(echo ${x%$TAB*})
		option=$(echo "${x#*$TAB}" | sed 's/ *$//g')
#		option=$(echo "${x#*$TAB}" | sed 's/ / /g')
		newchoices="${newchoices}${NL}${key}${TAB}${option}"
		if [ "$key" = "$default_choice" ]; then
		    default="$option"
		fi
	done
	choices="$newchoices"
	u=''
	IFS="$NL"
	# escape the commas and leading whitespace but keep them unescaped
	# in $choices
        for x in $choices; do
                u="$u, `echo ${x#*$TAB} | sed 's/,/\\\\,/g' | sed 's/^ /\\\\ /'`"
        done
        u=${u#, }
	if [ -n "$default" ]; then
	        db_set $template "$default"
	fi
	db_subst $template CHOICES "$u"
	code=0
	db_input $priority $template || code=1
	db_go || return 255
	db_get $template
	IFS="$NL"
	for x in $choices; do
		if [ "$RET" = "${x#*$TAB}" ]; then
			RET="${x%$TAB*}"
			break
		fi
	done
	return $code
}

menudir_default_choice () {
    local dir plugin
    dir="$1"
    plugin="$dir/[0-9]*$2"
    shift 2
    if [ ! -x $plugin/choices ]; then
        return 1
    fi
    name=$(basename $plugin)
    IFS="$NL"
    for option in $($plugin/choices "$@"); do
        printf "%s__________%s\n" $name $(echo "$option" | cut -f 1) > $dir/default_choice
    done
    restore_ifs
}

ask_user () {
    local IFS dir template priority default choices plugin name option
    dir="$1"; shift
    template=$(cat $dir/question)
    priority=$(cat $dir/priority)
    if [ -f $dir/default_choice ]; then
	default=$(cat $dir/default_choice)
    else
	default=""
    fi
    choices=$(
	for plugin in $dir/*; do
	    [ -d $plugin ] || continue
	    name=$(basename $plugin)
	    IFS="$NL"
	    for option in $($plugin/choices "$@"); do
		printf "%s__________%s\n" $name "$option"
	    done
	    restore_ifs
	done
    )
    db_fset $template seen false
    code=0
    debconf_select $priority $template "$choices" "$default" || code=$?
    if [ $code -ge 100 ]; then return 255; fi
    echo "$RET" >$dir/default_choice
    $dir/${RET%__________*}/do_option ${RET#*__________} "$@" || return $?
    return 0
}

partition_tree_choices () {
    local IFS
    for dev in $DEVICES/*; do
	[ -d $dev ] || continue
	printf "%s//\t%s\n" $dev "$(device_name $dev)" # GETTEXT?
	cd $dev

	open_dialog PARTITIONS
	partitions="$(read_paragraph)"
	close_dialog
	
	IFS="$TAB"
	echo "$partitions" |
	while { read num id size type fs path name; [ "$id" ]; }; do
	    part=${dev}/$id
	    [ -f $part/view ] || continue
	    printf "%s//%s\t        %s\n" "$dev" "$id" $(cat $part/view)
	done
	restore_ifs
    done
}

longint_le () {
	local x y
	# remove the leading 0
	x=$(expr "$1" : '0*\(.*\)')
	y=$(expr "$2" : '0*\(.*\)')
	if [ ${#x} -lt ${#y} ]; then
		return 0
	elif [ ${#x} -gt ${#y} ]; then
		return 1
	elif [ "$x" '<' "$y" ]; then
		return 0
	else
		return 1
	fi
}

longint2human () {
	local longint suffix bytes int frac deci
	# fallback value for $deci:
	deci="${deci:-.}"
	case ${#1} in
	    1|2|3)
		suffix=B
		longint=${1}0
		;;
	    4|5|6)
		suffix=kB
		longint=${1%??}
		;;
	    7|8|9)
		suffix=MB
		longint=${1%?????}
		;;
	    10|11|12)
		suffix=GB
		longint=${1%????????}
		;;
	    *)
		suffix=TB
		longint=${1%???????????}
		;;
	esac
	int=${longint%?}
	frac=${longint#$int}
	printf "%i%s%i %s\n" $int $deci $frac $suffix
}

human2longint () {
	local human suffix int frac longint
	set -- $*; human="$1$2$3$4$5" # without the spaces
	human=${human%b} #remove last b
	human=${human%B} #remove last B
	suffix=${human#${human%?}} # the last symbol of $human
	case $suffix in
	k|K|m|M|g|G|t|T)
		human=${human%$suffix}
		;;
	*)
		suffix=''
		;;
	esac
	int="${human%[.,]*}"
	[ "$int" ] || int=0
	frac=${human#$int}
	frac="${frac#[.,]}0000" # to be sure there are at least 4 digits
	frac=${frac%${frac#????}} # only the first 4 digits of $frac
	longint=$(($int * 10000 + $frac))
	case $suffix in
	k|K)
		longint=${longint%?}
		;;
	m|M)
		longint=${longint}00
		;;
	g|G)
		longint=${longint}00000
		;;
	t|T)
		longint=${longint}00000000
		;;
	*) # bytes (no suffix)
		longint=${longint%????}
		[ "$longint" ] || longint=0
	esac
	echo $longint
}

valid_human () {
	local IFS patterns
	patterns='[0-9][0-9]*$
[0-9][0-9]* *[bB]$
[0-9][0-9]* *[kKmMgGtT]$
[0-9][0-9]* *[kKmMgGtT][bB]$
[0-9]*[.,][0-9]*$
[0-9]*[.,][0-9]* *[bB]$
[0-9]*[.,][0-9]* *[kKmMgGtT]$
[0-9]*[.,][0-9]* *[kKmMgGtT][bB]$'
	IFS="$NL"
	for regex in $patterns; do
		if expr "$1" : "$regex" >/dev/null; then return 0; fi
	done
	return 1
}

update_partition () {
    local u
    cd $1
    open_dialog PARTITION_INFO $2
    read_line part
    close_dialog
    for u in /lib/partman/update.d/*; do
	[ -x "$u" ] || continue
	$u $1 $part
    done
}
        
DEVICES=/var/lib/partman/devices

# 0, 1 and 2 are standard input, output and error.
# 3, 4 and 5 are used by cdebconf
# 6=infifo
# 7=outfifo

open_infifo() {
    exec 6>/var/lib/partman/infifo
}

close_infifo() {
    exec 6>&-
}

open_outfifo () {
    exec 7</var/lib/partman/outfifo
}

close_outfifo () {
    exec 7<&-
}

write_line () {
    log IN: "$@"
    echo "$@" >&6
}

read_line () {
    read "$@" <&7
}

synchronise_with_server () {
    exec 6>/var/lib/partman/stopfifo
    exec 6>&-
}

read_paragraph () {
    local line
    while { read_line line; [ "$line" ]; }; do
	log "paragraph: $line"
	echo "$line"
    done
}

read_list () {
    local item list
    list=''
    while { read_line item; [ "$item" ]; }; do
	log "option: $item"
	if [ "$list" ]; then
	    list="$list, $item"
	else
	    list="$item"
	fi
    done
    echo "$list"
}

name_progress_bar () {
    echo $1 >/var/lib/partman/progress_info
}

error_handler () {
    local exception_type info state frac type priority message options skipped
    while { read_line exception_type; [ "$exception_type" != OK ]; }; do
	log error_handler: exception with type $exception_type
	case "$exception_type" in
	    Timer)
		if [ -f /var/lib/partman/progress_info ]; then
		    info=$(cat /var/lib/partman/progress_info)
		else
		    info=partman/processing
		fi
		db_progress START 0 1000 partman/text/please_wait
		db_progress INFO $info
		while { read_line frac state; [ "$frac" != ready ]; }; do
		    if [ "$state" ]; then
			db_subst $info STATE "$state" 
			db_progress INFO $info
		    fi
		    db_progress SET $frac
		done
		db_progress STOP
		continue
		;;
	    Information)
		type='Information'
		priority=low
		;;
	    Warning)
		type='Warning!'
		priority=medium
		;;
	    Error)
		type='ERROR!!!'
		priority=high
		;;
	    Fatal)
		type='FATAL ERROR!!!'
		priority=critical
		;;
	    Bug)
		type='A bug has been discovered!!!'
		priority=critical
		;;
	    No?Implementation)
		type='Not yet implemented!'
		priority=critical
		;;
	    *)
		type="??? $exception_type ???"
		priority=critical
		;;
	esac
	log error_handler: reading message
	message=$(read_paragraph)
	log error_handler: reading options
	options=$(read_list)
	db_subst partman/exception_handler TYPE "$type"
	db_subst partman/exception_handler DESCRIPTION "$message"
	db_fset partman/exception_handler seen false
	db_subst partman/exception_handler CHOICES "$options"
	if
	    expr "$options" : '.*,.*' >/dev/null \
	    && db_input $priority partman/exception_handler
	then
	    if db_go; then
		db_get partman/exception_handler
		write_line "$RET"
	    else
		write_line "unhandled"
	    fi
	else
	    write_line "unhandled"
	    db_subst partman/exception_handler_note TYPE "$type"
	    db_subst partman/exception_handler_note DESCRIPTION "$message"
	    db_fset partman/exception_handler_note seen false
	    db_input $priority partman/exception_handler_note || true
	    db_go || true
	fi
    done
    if [ -f /var/lib/partman/progress_info ]; then
	rm /var/lib/partman/progress_info
    fi
}

open_dialog () {
    command="$1"
    shift
    open_infifo
    write_line "$command" "${PWD##*/}" "$@"
    open_outfifo
    error_handler
}

close_dialog () {
    close_outfifo
    close_infifo
    exec 6>/var/lib/partman/stopfifo
    exec 6>&-
    exec 7>/var/lib/partman/outfifo
    exec 7>&-
    exec 6>/var/lib/partman/stopfifo
    exec 6>&-
    exec 6</var/lib/partman/infifo
    cat <&6 >/dev/null
    exec 6<&-
    exec 6>/var/lib/partman/stopfifo
    exec 6>&-
}

log () {
    local program
    echo $0: "$@" >>/var/log/partman
}

####################################################################
# The functions below are not yet documented
####################################################################

# TODO: this should not be global
humandev () {
    local host bus target part lun idenum targtype scsinum
    case "$1" in
	/dev/ide/host*/bus[01]/target[01]/lun0/disc)
	    host=`echo $1 | sed 's,/dev/ide/host\(.*\)/bus.*/target[01]/lun0/disc,\1,'`
	    bus=`echo $1 | sed 's,/dev/ide/host.*/bus\(.*\)/target[01]/lun0/disc,\1,'`
	    target=`echo $1 | sed 's,/dev/ide/host.*/bus.*/target\([01]\)/lun0/disc,\1,'`
	    idenum=$((2 * $host + $bus + 1))
	    if [ "$target" = 0 ]; then
		db_metaget partman/text/ide_master_disk description
		printf "$RET" ${idenum}
	    else
		db_metaget partman/text/ide_slave_disk description
		printf "$RET" ${idenum}
	    fi
	    ;;
	/dev/ide/host*/bus[01]/target[01]/lun0/part*)
	    host=`echo $1 | sed 's,/dev/ide/host\(.*\)/bus.*/target[01]/lun0/part.*,\1,'`
	    bus=`echo $1 | sed 's,/dev/ide/host.*/bus\(.*\)/target[01]/lun0/part.*,\1,'`
	    target=`echo $1 | sed 's,/dev/ide/host.*/bus.*/target\([01]\)/lun0/part.*,\1,'`
	    part=`echo $1 | sed 's,/dev/ide/host.*/bus.*/target[01]/lun0/part\(.*\),\1,'`
	    idenum=$((2 * $host + $bus + 1))
	    if [ "$target" = 0 ]; then
		db_metaget partman/text/ide_master_partition description
		printf "$RET" ${idenum} "$part"
	    else
		db_metaget partman/text/ide_slave_partition description
		printf "$RET" ${idenum} "$part"
	    fi
	    ;;
	/dev/scsi/host*/bus*/target*/lun*/disc)
	    host=`echo $1 | sed 's,/dev/scsi/host\(.*\)/bus.*/target.*/lun.*/disc,\1,'`
	    bus=`echo $1 | sed 's,/dev/scsi/host.*/bus\(.*\)/target.*/lun.*/disc,\1,'`
	    target=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target\(.*\)/lun.*/disc,\1,'`
	    lun=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target.*/lun\(.*\)/disc,\1,'`
	    scsinum=$(($host + 1))
	    db_metaget partman/text/scsi_disk description
	    printf "$RET" ${scsinum} ${bus} ${target} ${lun}
	    ;;
	/dev/scsi/host*/bus*/target*/lun*/part*)
	    host=`echo $1 | sed 's,/dev/scsi/host\(.*\)/bus.*/target.*/lun.*/part.*,\1,'`
	    bus=`echo $1 | sed 's,/dev/scsi/host.*/bus\(.*\)/target.*/lun.*/part.*,\1,'`
	    target=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target\(.*\)/lun.*/part.*,\1,'`
	    lun=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target.*/lun\(.*\)/part.*,\1,'`
	    part=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target.*/lun.*/part\(.*),\1,'`
	    scsinum=$(($host + 1))
	    db_metaget partman/text/scsi_partition description
	    printf "$RET" ${scsinum} ${bus} ${target} ${lun} ${part}
	    ;;
	*)
	    echo "$1"
	    ;;
    esac
}

device_name () {
    cd $1
    printf "%s - %s %s" "$(humandev $(cat device))" "$(longint2human $(cat size))" "$(cat model)"
}

enable_swap () {
    local swaps dev num id size type fs path name method filesystem
    # do swapon only when we will be able to swapoff afterwards
    [ -f /proc/swaps ] || return 0
    swaps=''
    for dev in $DEVICES/*; do
	[ -d $dev ] || continue
	cd $dev
	open_dialog PARTITIONS
	while { read_line num id size type fs path name; [ "$id" ]; }; do
	    [ $fs != free ] || continue
	    [ -f "$id/method" ] || continue
	    [ -f "$id/acting_filesystem" ] || continue
	    filesystem=$(cat $id/acting_filesystem)
	    if [ "$filesystem" = linux-swap ]; then
		swaps="$swaps $path"
	    fi
	done
	close_dialog
    done
    for path in $swaps; do
	swapon $path
    done
}

disable_swap () {
    [ -f /proc/swaps ] || return 0
    cat /proc/swaps | grep '^/dev' \
	| while read path x; do
	      swapoff $path
          done
}

default_disk_label () {
    if [ -x /bin/archdetect ]; then
	archdetect=$(archdetect)
    else
	archdetect=unknown/generic
    fi
    arch=${archdetect%/*}
    sub=${archdetect#*/}
    case "$arch" in
	alpha)
	    # Load srm_env.o if we can; this should fail on ARC-based systems.
	    (modprobe srm_env || true) 2> /dev/null
	    if [ -f /proc/srm_environment/named_variables/booted_dev ]; then
                # Have SRM, so need BSD disklabels
		echo bsd
	    else
		echo msdos
	    fi;;	    
	arm)
	    echo msdos;;
	amd64)
	    echo msdos;;
	hppa)
	    echo msdos;;
	ia64)
	    echo gpt;;
	i386)
	    echo msdos;;
	m68k)
	    case "$sub" in
		amiga)
		    echo amiga;;
		atari)
		    echo UNSUPPORTED;; # atari is unsupported by parted
		mac)
		    echo mac;;
		*vme*)
		    echo msdos;;
		*)
		    echo UNKNOWN;;
	    esac;;
	mips)
	    echo dvh;;
	mipsel)
	    echo msdos;;
	powerpc)
	    case "$sub" in
		apus)
		    echo amiga;;
		chrp)
		    echo msdos;; # guess
		chrp_rs6k)
		    echo msdos;; # guess
		chrp_pegasos)
		    echo amiga;;
		prep)
		    echo msdos;; # guess
		newworld)
		    echo mac;;
		oldworld)
		    echo mac;;
		*)
		    echo UNKNOWN;;
	    esac;;
	s390)
	    echo UNSUPPORTED;; # ibm is unsupported by parted
	sparc)
	    echo sun;;
	*)
	    echo UNKNOWN;;
    esac
}

# Local Variables:
# coding: utf-8
# End:
