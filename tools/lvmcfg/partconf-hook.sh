#!/bin/sh

[ -z "$1" ] && exit 0

vgdisplay -v | grep -q "$1"
[ $? -eq 0 ] && exit 1

exit 0

