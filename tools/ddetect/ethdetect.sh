#!/bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

is_not_loaded() {
    module="$1"
    if cut -d" " -f1 /proc/modules | grep -q "^${module}\$" ; then
	false
    else
	true
    fi
}

module_probe() {
    module="$1"
    db_subst ethdetect/module_params MODULE "$module"
    db_input low ethdetect/module_params || [ $? -eq 30 ]
    db_go
    db_get ethdetect/module_params
    if modprobe -v "$module" $RET ; then
	if [ "$RET" != "" ]; then
		register-module "$module" $RET
	fi
    else
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
		CHOICES="$mod, $CHOICES"
	fi
    done

    db_subst ethdetect/module_select CHOICES "$CHOICES"
    db_input high ethdetect/module_select || [ $? -eq 30 ]
    db_go || break

    db_get ethdetect/module_select
    if [ "$RET" = "none of the above" ]; then
	    exit 1
    fi
    module="$RET"
    if [ -n "$module" ] && is_not_loaded "$module" ; then
	register-module "$module"
	module_probe "$module"
    fi
    
    # No ethernet interface. Try manual loading.
    db_fset ethdetect/cannot_find seen false
    db_input high ethdetect/cannot_find
    db_go || break
done
