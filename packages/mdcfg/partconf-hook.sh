#!/bin/sh

[ -z "$1" ] && exit 0

DEVICE=`echo $1 | sed -e "s/\/dev\///"`

echo ${DEVICE}

if grep -q "${DEVICE}" /proc/mdstat; then
  exit 1
else
  exit 0
fi

