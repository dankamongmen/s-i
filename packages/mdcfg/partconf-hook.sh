#!/bin/sh

[ -z "$1" ] && exit 0

DEVICE=`echo $1 | sed -e "s/\/dev\///"`

echo ${DEVICE}

cat /proc/mdstat | grep -q "${DEVICE}"
[ $? -eq 0 ] && exit 1

exit 0

