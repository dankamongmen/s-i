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

module_insmod() {
    module="$1"
    db_subst ethdetect/module_params MODULE "$module"
    db_input low ethdetect/module_params || [ $? -eq 30 ]
    db_go
    db_get ethdetect/module_params
    if modprobe -v "$module" $RET ; then
	prebaseconfig=/usr/lib/prebaseconfig.d/40ethdetect
	echo "echo \"$module $RET\" >> /target/etc/modules" >> $prebaseconfig
    else
	db_subst ethdetect/modprobe_error CMD_LINE_PARAM "modprobe -v $module"
	db_input critical ethdetect/modprobe_error || [ $? -eq 30 ]
	db_go
	false
    fi
}

db_title "Network Hardware Configuration"

db_input low ethdetect/detection_type || [ $? -eq 30 ]
db_go

db_get ethdetect/detection_type
if [ true = "$RET" ] ; then
    # Autodetect using hw-detect from hw-detect
    hw-detect || true
fi

if [ -z "`sed -e "s/lo://" < /proc/net/dev | grep "[a-z0-9]*:[ ]*[0-9]*"`"  ]
then
    # No ethernet card.  Try manual loading
    db_input high ethdetect/module_select || [ $? -eq 30 ]
    db_go

    db_get ethdetect/module_select
    if [ other = "$RET" ] ; then
	db_input high ethdetect/module_prompt || [ $? -eq 30 ]
	db_go
	db_get ethdetect/module_prompt
    fi
    module="$RET"
    if is_not_loaded "$module" ; then
	module_insmod "$module"
    fi
fi
