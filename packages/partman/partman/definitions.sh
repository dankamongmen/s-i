
. /usr/share/debconf/confmodule

NBSP=' '
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
	# Debconf ignores spaces so we have to remove them from $choices
	newchoices=''
	IFS="$NL"
	for x in $choices; do
		local key option
		restore_ifs
		key=$(echo ${x%$TAB*})
		# work around bug #243373
		if [ "$TERM" = xterm -o "$TERM" = bterm ]; then
			debconf_select_lead="$NBSP"
		else
			debconf_select_lead="> "
		fi
		option=$(echo "${x#*$TAB}" | sed 's/ *$//g' | sed "s/^ /$debconf_select_lead/g")
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
    printf "%s__________%s\n" "$(basename $1/??$2)" "$3" > $1/default_choice
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
    code=0
    debconf_select $priority $template "$choices" "$default" || code=$?
    if [ $code -ge 100 ]; then return 255; fi
    echo "$RET" >$dir/default_choice
    $dir/${RET%__________*}/do_option ${RET#*__________} "$@" || return $?
    return 0
}

partition_tree_choices () {
    local IFS
    local whitespace_hack=""
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
	    printf "%s//%s\t     %s\n" "$dev" "$id" $(cat $part/view)
	done
	restore_ifs
    done | while read line; do
        # A hack to make sure each line in the table is unique and
	# selectable by debconf -- pad lines with varying amounts of
	# whitespace.
    	whitespace_hack="$NBSP$whitespace_hack"
	echo "$line$whitespace_hack"
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
	elif [ "$x" = "$y" ]; then
	    return 0
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
		longint=${1}00
		;;
	    4|5|6)
		suffix=kB
		longint=${1%?}
		;;
	    7|8|9)
		suffix=MB
		longint=${1%????}
		;;
	    10|11|12)
		suffix=GB
		longint=${1%???????}
		;;
	    *)
		suffix=TB
		longint=${1%??????????}
		;;
	esac
	longint=$(($longint + 5))
	longint=${longint%?}
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
	*) # no suffix:
		# bytes
		#longint=${longint%????}
		#[ "$longint" ] || longint=0
		# megabytes
		longint=${longint}00
		;;
	esac
	echo $longint
}

valid_human () {
	local IFS patterns
	patterns='[0-9][0-9]* *$
[0-9][0-9]* *[bB] *$
[0-9][0-9]* *[kKmMgGtT] *$
[0-9][0-9]* *[kKmMgGtT][bB] *$
[0-9]*[.,][0-9]* *$
[0-9]*[.,][0-9]* *[bB] *$
[0-9]*[.,][0-9]* *[kKmMgGtT] *$
[0-9]*[.,][0-9]* *[kKmMgGtT][bB] *$'
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
    [ "$part" ] || return 0
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
	    db_subst partman/exception_handler_note TYPE "$type"
	    db_subst partman/exception_handler_note DESCRIPTION "$message"
	    db_input $priority partman/exception_handler_note || true
	    db_go || true
	    write_line "unhandled"
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
    local host bus target part lun idenum targtype scsinum linux
    case "$1" in
	/dev/ide/host*/bus[01]/target[01]/lun0/disc)
	    host=`echo $1 | sed 's,/dev/ide/host\(.*\)/bus.*/target[01]/lun0/disc,\1,'`
	    bus=`echo $1 | sed 's,/dev/ide/host.*/bus\(.*\)/target[01]/lun0/disc,\1,'`
	    target=`echo $1 | sed 's,/dev/ide/host.*/bus.*/target\([01]\)/lun0/disc,\1,'`
	    idenum=$((2 * $host + $bus + 1))
	    linux=$(mapdevfs $1)
	    linux=${linux#/dev/}
	    if [ "$target" = 0 ]; then
		db_metaget partman/text/ide_master_disk description
		printf "$RET" ${idenum} ${linux}
	    else
		db_metaget partman/text/ide_slave_disk description
		printf "$RET" ${idenum} ${linux}
	    fi
	    ;;
	/dev/ide/host*/bus[01]/target[01]/lun0/part*)
	    host=`echo $1 | sed 's,/dev/ide/host\(.*\)/bus.*/target[01]/lun0/part.*,\1,'`
	    bus=`echo $1 | sed 's,/dev/ide/host.*/bus\(.*\)/target[01]/lun0/part.*,\1,'`
	    target=`echo $1 | sed 's,/dev/ide/host.*/bus.*/target\([01]\)/lun0/part.*,\1,'`
	    part=`echo $1 | sed 's,/dev/ide/host.*/bus.*/target[01]/lun0/part\(.*\),\1,'`
	    idenum=$((2 * $host + $bus + 1))
	    linux=$(mapdevfs $1)
	    linux=${linux#/dev/}
	    if [ "$target" = 0 ]; then
		db_metaget partman/text/ide_master_partition description
		printf "$RET" ${idenum} "$part" "${linux}"
	    else
		db_metaget partman/text/ide_slave_partition description
		printf "$RET" ${idenum} "$part" "${linux}"
	    fi
	    ;;
	/dev/scsi/host*/bus*/target*/lun*/disc)
	    host=`echo $1 | sed 's,/dev/scsi/host\(.*\)/bus.*/target.*/lun.*/disc,\1,'`
	    bus=`echo $1 | sed 's,/dev/scsi/host.*/bus\(.*\)/target.*/lun.*/disc,\1,'`
	    target=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target\(.*\)/lun.*/disc,\1,'`
	    lun=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target.*/lun\(.*\)/disc,\1,'`
	    scsinum=$(($host + 1))
	    linux=$(mapdevfs $1)
	    linux=${linux#/dev/}
	    db_metaget partman/text/scsi_disk description
	    printf "$RET" ${scsinum} ${bus} ${target} ${lun} ${linux}
	    ;;
	/dev/scsi/host*/bus*/target*/lun*/part*)
	    host=`echo $1 | sed 's,/dev/scsi/host\(.*\)/bus.*/target.*/lun.*/part.*,\1,'`
	    bus=`echo $1 | sed 's,/dev/scsi/host.*/bus\(.*\)/target.*/lun.*/part.*,\1,'`
	    target=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target\(.*\)/lun.*/part.*,\1,'`
	    lun=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target.*/lun\(.*\)/part.*,\1,'`
	    part=`echo $1 | sed 's,/dev/scsi/host.*/bus.*/target.*/lun.*/part\(.*),\1,'`
	    scsinum=$(($host + 1))
	    linux=$(mapdevfs $1)
	    linux=${linux#/dev/}
	    db_metaget partman/text/scsi_partition description
	    printf "$RET" ${scsinum} ${bus} ${target} ${lun} ${part} ${linux}
	    ;;
	/dev/cciss/*)
	    # /dev/cciss/hostN/targetM/disc is 2.6 form
	    # /dev/cciss/discM/disk seems to be 2.4 form
	    line=`echo $1 | sed 's,/dev/cciss/\([a-z]*\)\([0-9]*\)/\(.*\),\1 \2 \3,'`
	    cont=`echo "$line" | cut -d" " -f2`
	    host=`echo "$line" | cut -d" " -f1`
	    line=`echo "$line" | cut -d" " -f3`
	    if [ "$host" = host ] ; then
	       line=`echo "$line" | sed 's,target\([0-9]*\)/\([a-z]*\)\(.*\),\1 \2 \3,'`
	       lun=`echo  "$line" | cut -d" " -f1`
	       disc=`echo "$line" | cut -d" " -f2`
	       part=`echo "$line" | cut -d" " -f3`
	    else
	       line=`echo "$line" | sed 's,disc\([0-9]*\)/\([a-z]*\)\(.*\),\1 \2 \3,'`
	       lun=`echo  "$line" | cut -d" " -f1`
	       if [ "$lun" > 15 ] ; then
	          cont=$(($lun / 16))
		  lun=$(($lun % 16))
	       else
		  cont=0
	       fi
	       disc=`echo "$line" | cut -d" " -f2`
	       part=`echo "$line" | cut -d" " -f3`
	    fi
	    linux=$(mapdevfs $1)
	    linux=${linux#/dev/}
	    if [ "$disc" = disc ] ; then
	       db_metaget partman/text/scsi_disk description
	       printf "$RET" ".CCISS" "-" ${cont} ${lun} ${linux}
	    else
	       db_metaget partman/text/scsi_partition description
	       printf "$RET" ".CCISS" "-" ${cont} ${lun} ${part} ${linux}
	    fi
	    ;;
	/dev/md/*)
	    device=`echo "$1" | sed -e "s/.*md\/\?\(.*\)/\1/"`
	    type=`grep "^md${device}[ :]" /proc/mdstat | sed -e "s/^.* : active raid\([[:alnum:]]\).*/\1/"`
	    db_metaget partman/text/raid_device description
	    printf "$RET" ${type} ${device}
	    ;;
	/dev/mapper/*)
	    # LVM2 devices are found as /dev/mapper/<vg>-<lv>.  If the vg
	    # or lv contains a dash, the dash is replaced by two dashes.
	    # In order to decode this into vg and lv, first find the
	    # occurance of one single dash to split the string into vg and
	    # lv, and then replace two dashes next to each other with one.
	    vglv=${1#/dev/mapper/}
	    vglv=`echo "$vglv" | sed -e 's/\([^-]\)-\([^-]\)/\1 \2/' | sed -e 's/--/-/g'`
	    vg=`echo "$vglv" | cut -d" " -f1`
	    lv=`echo "$vglv" | cut -d" " -f2`
	    db_metaget partman/text/lvm_lv description
	    printf "$RET" $vg $lv
	    ;;
	*)
	    # Check if it's an LVM1 device
	    vg=`echo "$1" | sed -e 's,/dev/\([^/]\+\).*,\1,'`
	    lv=`echo "$1" | sed -e 's,/dev/[^/]\+/,,'`
	    if [ -e "/proc/lvm/VGs/$vg/LVs/$lv" ] ; then
		db_metaget partman/text/lvm_lv description
		printf "$RET" $vg $lv
	    else
		echo "$1"
	    fi
	    ;;
    esac
}

device_name () {
    cd $1
    printf "%s - %s %s" "$(humandev $(cat device))" "$(longint2human $(cat size))" "$(cat model)"
}

enable_swap () {
    local swaps dev num id size type fs path name method
    local startdir="$(pwd)"
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
	    method=$(cat $id/method)
	    if [ "$method" = swap ]; then
		swaps="$swaps $path"
	    fi
	done
	close_dialog
    done
    for path in $swaps; do
	swapon $path || true
    done
    cd "$startdir"
}

disable_swap () {
    [ -f /proc/swaps ] || return 0
    grep '^/dev' /proc/swaps \
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
	    case "$sub" in
		riscstation)
		    echo msdos;;
		netwinder)
		    echo msdos;;
		bast)
		    echo msdos;;
		*)
		    echo UNKNOWN;;
	    esac;;
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
		q40)
		    echo UNSUPPORTED;; # (same as atari)
		sun*)
	    	    echo sun;;
		*)
		    echo UNKNOWN;;
	    esac;;
	mips)
	    case "$sub" in
		# Indy
		r4k-ip22 | r5k-ip22 | r8k-ip26 | r10k-ip28)
		    echo dvh;;
		# Origin
		r10k-ip27 | r12k-ip27)
		    echo dvh;;
		# O2
		r5k-ip32 | r10k-ip32 | r12k-ip32)
		    echo dvh;;
		# SiByte SWARM
		sb1-swarm-bn)
		    echo msdos;;
		*)
		    echo UNKNOWN;;
	    esac;;
	mipsel)
	    case "$sub" in
		r3k-kn02)
		    echo msdos;;
		r4k-kn04)
		    echo msdos;;
		sb1-swarm-bn)
		    echo msdos;;
		cobalt)
		    echo msdos;;
		*)
		    echo UNKNOWN;;
	    esac;;
	powerpc)
	    case "$sub" in
		apus)
		    echo amiga;;
		amiga)
		    echo amiga;;
		chrp)
		    echo msdos;; # guess
		chrp_rs6k)
		    echo msdos;; # guess
		chrp_pegasos)
		    echo amiga;;
		prep)
		    echo msdos;; # guess
		powermac_newworld)
		    echo mac;;
		powermac_oldworld)
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

log '*******************************************************'

# Local Variables:
# coding: utf-8
# End:
