
TAB='	'

# Print a list of the names of all volume groups, one per line
# VG name<TAB>size
VG_list () {
    local name size
    vgdisplay | grep '^[ ]*VG Name' | sed -e 's/.*[[:space:]]\+\(.*\)$/\1/' | sort | 
    while read name; do
	# TODO: the size is in B, KB or MB?
	size=$(vgdisplay "$name" 2>&1 | grep '^[ ]*VG Size' | sed -e 's/^[ ]*VG Size \+//')
	echo "${name}${TAB}${size}"
    done
}

# Create a new VG from a list of PV.  The PV will be pvcreate-d
# $1 = the name
# $2,$3,$4,.. = a non-empty list of PV
VG_create () {
    local vg
    vg="$1"
    shift
    log-output -t partman-auto-lvm pvcreate -ff -y $*
    log-output -t partman-auto-lvm vgcreate "$vg" $* || return 1
    return 0
}

# Print the name of the VG of a PV; exit 1 if this is not a phisical
# volume or the information is not available
# $1 = the name of the PV
VG_of_PV () {
    local cmdout
    cmdout=`pvdisplay "$1" 2>&1`
    if echo "$cmdout" | grep -q 'is a new physical volume'; then
	return 1
    fi
    
    if echo "$cmdout" | grep -q '^pvdisplay'; then
	return 1
    fi
    VG=`echo "$cmdout" | grep '^[ ]*VG Name' | \
                sed -e 's/^[ ]*VG Name \+//'`
    if [ "$VG" != "" ]; then
	echo "$VG"
	return 0
    else
	return 1
    fi
}

# Make a PV not to be part of its VG
# $1 = PV
VG_reduce () {
    local vg
    vg=`VG_of_PV $1` \
	&& log-output -t partman-auto-lvm vgreduce $vg $1
}

# Add a new PV to a VG.  The PV will be pvcreate-d
# $1 = PV
# $2 = VG
VG_extend () {
    log-output -t partman-auto-lvm pvcreate -ff -y $1 \
	&& log-output -t partman-auto-lvm vgextend $2 $1
}

# Create a new LV in a VG
# $1 = VG
# $2 = size in bytes
# $3 = name of the LV to create
LV_create () {
#    if [ "$2" = full ]; then
# Using full VG not implemented until we have a way to get the free size of
# a VG
#	log-output -t partman-auto-lvm lvcreate -l$() -n $3 $1
#   else
	log-output -t partman-auto-lvm lvcreate -L${2%???}k -n $3 $1
#    fi
}

# Delete a LV
# $1 = VG
# $2 = LV
LV_remove () {
    log-output -t partman-auto-lvm lvremove -f /dev/$1/$2
}

# Print a list of all LV in a VG, one per line using the following format:
# VG name<TAB>size
# $1 = VG
LV_list () {
    local i cmdout size
    for i in \
        $(vgdisplay -v $1 | grep '^[ ]*LV Name' |
	    sed -e 's,.*/\(.*\),\1,' | sort)
    do
	cmdout=`lvdisplay "$1" 2>&1`
        size=`echo "$cmdout" | grep '^[ ]*LV Size' | \
                sed -e 's/^[ ]*LV Size \+\(.*\)/\1/'`
	echo "${i}${TAB}${size}"
    done
}
