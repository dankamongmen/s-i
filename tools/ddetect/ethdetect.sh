#!/bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

is_not_loaded() {
    local module="$1"
    if cut -d" " -f1 /proc/modules | grep -q "^${module}\$" ; then
	false
    else
	true
    fi
}

load_module() {
	local module="$1"
   	local priority=low
    
	case "$module" in
	"plip")
		module_probe parport_pc high
		priority=high		
		;;
	"ne")
		priority=high
		;;
	esac
	
	module_probe "$module" "$priority"
}

module_probe() {
    local module="$1"
    local priority="$2"
    local template="ethdetect/module_params"
    local question="$template/$module"

    db_register "$template" "$question"
    db_subst "$question" MODULE "$module"

    db_input $priority "$question" || [ $? -eq 30 ]
    db_go
    db_get "$question"
    if modprobe -v "$module" $RET ; then
	if [ "$RET" != "" ]; then
		register-module "$module" $RET
	fi
    else
	db_unregister "$question"
	db_subst ethdetect/modprobe_error CMD_LINE_PARAM "modprobe -v $module"
	db_input critical ethdetect/modprobe_error || [ $? -eq 30 ]
	db_go
	false
    fi
}

db_settitle debian-installer/ethdetect/title

db_input low ethdetect/detection_type || [ $? -eq 30 ]
db_go

db_get ethdetect/detection_type
if [ true = "$RET" ] ; then
    # Autodetect using hw-detect from hw-detect
    hw-detect || true
fi

while [ -z "`sed -e "s/lo://" < /proc/net/dev | grep "[a-z0-9]*:[ ]*[0-9]*"`" ]
do
    CHOICES=""
    for mod in $(find /lib/modules/*/kernel/drivers/net -type f | sed 's/\.o$//' | sed 's/\.ko$//' | sed 's/.*\///' | sort); do
	if [ -z "$CHOICES" ]; then
		CHOICES="$mod"
	else
		CHOICES="$CHOICES, $mod"
	fi
    done

    if [ -n "$CHOICES" ]; then
    	db_subst ethdetect/module_select CHOICES "$CHOICES"
    	db_input high ethdetect/module_select || [ $? -eq 30 ]
    	db_go || break

        db_get ethdetect/module_select
        if [ "$RET" = "none of the above" ]; then
    	    exit 1
        fi
        module="$RET"
        if [ -n "$module" ] && is_not_loaded "$module" ; then
		load_module "$module"
        fi
	continue
    fi
    
    # No ethernet interface. Try manual loading.
    db_fset ethdetect/cannot_find seen false
    db_input high ethdetect/cannot_find
    db_go || break

    if [ -z "$CHOICES" ]; then
	    exit 1
    fi
done
