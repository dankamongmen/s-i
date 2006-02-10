
TAB='	'

# Print a list of the names of all physical volumes and the volume groups they contain
PV_list () {
	pvs -o pv_name,vg_name --noheadings --sort pv_name,vg_name --separator "${TAB}"
}

# Print a list of the names of all volume groups and their size in M (including decimals!)
VG_list () {
	vgs -o vg_name,vg_size --nosuffix --units M --noheadings --sort vg_name --separator "${TAB}"
}

# Create a new VG from a list of PV.  The PV will be pvcreate-d
# $1 = the name
# $2,$3,$4,.. = a non-empty list of PV
VG_create () {
	local vg
	vg="$1"
	shift
	log-output -t partman-auto-lvm pvcreate -ff -y $* || return 1
	log-output -t partman-auto-lvm vgcreate "$vg" $* || return 1
	return 0
}

# Print the name of the VG of which a PV belongs to.
# $1 = the name of the PV
VG_of_PV () {
	pvs -o vg_name $1 --noheadings --separator "${TAB}" 2>/dev/null || return 1
}

# Make a PV not to be part of its VG
# $1 = PV
VG_reduce () {
	local vg
	vg=`VG_of_PV $1` && \
		log-output -t partman-auto-lvm vgreduce $vg $1
}

# Add a new PV to a VG.  The PV will be pvcreate-d
# $1 = PV
# $2 = VG
VG_extend () {
	log-output -t partman-auto-lvm pvcreate -ff -y $1 && \
		log-output -t partman-auto-lvm vgextend $2 $1
}

# Create a new LV in a VG
# $1 = VG
# $2 = size in bytes
# $3 = name of the LV to create
LV_create () {
	local size
	if [ "$2" = full ]; then
		size=$(vgs -o vg_free --noheadings --nosuffix --units k $1 | sed -e 's/\..*//g')
		if [ "$size" -le "0" ] || [ -z "$size" ]; then
			return 1
		fi
	else
		size=${2%???}
	fi
	log-output -t partman-auto-lvm lvcreate -L${size}k -n $3 $1
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
	lvs $1 -o lv_name,lv_size --nosuffix --units M --noheadings --sort lv_name --separator "${TAB}"
}
