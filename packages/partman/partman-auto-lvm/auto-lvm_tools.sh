. /lib/partman/definitions.sh
. /lib/partman/recipes.sh
. /lib/partman/lvm_tools.sh
. /lib/partman/auto-shared.sh

bail_out() {
	db_input critical partman-auto-lvm/$1 || true
	db_go || true
	exit 1
}

auto_lvm_prepare() {
	local dev method size free_size normalscheme target
	dev=$1
	method=$2

	[ -f $dev/size ] || return 1
	size=$(cat $dev/size)

	# Be sure the modules are loaded
	modprobe dm-mod >/dev/null 2>&1 || true
	modprobe lvm-mod >/dev/null 2>&1 || true

	if type update-dev >/dev/null 2>&1; then
		log-output -t update-dev update-dev
	fi

	target="$(humandev $(cat $dev/device)) - $(cat $dev/model)"
	target="$target: $(longint2human $size)"
	free_size=$(expr 0000000"$size" : '0*\(..*\)......$') # convert to megabytes

	choose_recipe lvm "$target" "$free_size" || return $?

	wipe_disk || return $?

	# Check if partition is usable; use existing partman-auto template as we depend on it
	if [ "$free_type" = unusable ]; then
		db_input critical partman-auto/unusable_space || true
		db_go || true
		return 1
	fi
	free_size=$(expr 0000000"$free_size" : '0*\(..*\)......$') # convert to megabytes

	decode_recipe $recipe lvm

	# Make sure the recipe contains lvmok tags
	if ! echo "$scheme" | grep -q "lvmok"; then
		bail_out unusable_recipe
	fi

	# Make sure a boot partition isn't marked as lvmok
	if echo "$scheme" | grep "lvmok" | grep -q "[[:space:]]/boot[[:space:]]"; then
		bail_out unusable_recipe
	fi

	### Situation
	### We have a recipe foo from arch bar. we don't know anything other than what
	### partitions can go on lvm ($lvmok{ } tag).
	### As output we need to have 2 recipes:
	### - recipe 1 (normalscheme) that will contain all non-lvm partitions including /boot.
	###            The /boot partition should already be defined in the schema.
	### - recipe 2 everything that can go on lvm and it's calculated in perform_recipe_by_lvm.

	# Get the scheme of partitions that must be created outside LVM
	normalscheme=$(echo "$scheme" | grep -v "lvmok")

	# Check if the scheme contains a boot partition; if not warn the user
	# Except for powerpc/prep as that has the kernel in the prep partition
	if type archdetect >/dev/null 2>&1; then
		archdetect=$(archdetect)
	else
		archdetect=unknown/generic
	fi

	case $archdetect in
	    powerpc/prep)
		: ;;
	    *)
		if ! echo "$normalscheme" | grep -q "[[:space:]]/boot[[:space:]]"; then
			db_input critical partman-auto-lvm/no_boot || true
			db_go || return 30
			db_get partman-auto-lvm/no_boot || true
			[ "$RET" = true ] || return 30
		fi
		;;
	esac

	# Creating envelope
	scheme="$normalscheme${NL}100 1000 1000000000 ext3 method{ $method }"

	expand_scheme

	clean_method

	create_primary_partitions

	# This variable will be used to store the partitions that will be LVM
	# by create_partitions; zero it to be sure it's not cluttered.
	# It will be used later to provide real paths to partitions to LVM.
	# (still one atm)
	devfspv_devices=''

	create_partitions

	if ! confirm_changes "partman-lvm"; then
		return 30
	fi

	# Write the partition tables
	disable_swap
	cd $dev
	open_dialog COMMIT
	close_dialog

	update_all
}

auto_lvm_perform() {
	# Use hostname as default vg name (if available)
	local defvgname
	db_get partman-auto-lvm/new_vg_name
	if [ -z "$RET" ]; then
		if [ -s /etc/hostname ]; then
			defvgname=$(cat /etc/hostname | head -n 1 | tr -d " ")
		fi
		if [ "$defvgname" ]; then
			db_set partman-auto-lvm/new_vg_name $defvgname
		else
			db_set partman-auto-lvm/new_vg_name Debian
		fi
	fi

	# Choose name, create VG and attach each partition as a physical volume
	noninteractive=true
	while true; do
		db_input medium partman-auto-lvm/new_vg_name || eval $noninteractive
		db_go || return 1
		db_get partman-auto-lvm/new_vg_name
		VG_name="$RET"

		# Check that the volume group name is not in use
		if ! vg_get_info "$VG_name"; then
			break
		fi
		noninteractive="bail_out vg_exists"
		db_register partman-auto-lvm/new_vg_name_exists partman-auto-lvm/new_vg_name
	done

	if vg_create "$VG_name" $pv_devices; then
		perform_recipe_by_lvm $VG_name $recipe
	else
		bail_out vg_create_error
	fi

	# Default to accepting the autopartitioning
	menudir_default_choice /lib/partman/choose_partition finish finish || true
}
