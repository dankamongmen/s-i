#! /bin/sh
set -e

# Detect hardware on the mac-io bus. This is a grab-bag of stuff at the
# moment; it should really move into discover or be hotpluggable.
#
# If the hardware is of use within d-i, then echo it and register-module it;
# otherwise, just use register-module.

if [ -f /proc/device-tree/aliases/mac-io ]; then
	macio="/proc/device-tree$(cat /proc/device-tree/aliases/mac-io)"
else
	exit 0
fi

for dir in $(find "$macio" -type d); do
	name="$(cat "$dir/name" 2>/dev/null || true)"
	device_type="$(cat "$dir/device_type" 2>/dev/null || true)"
	compatible="$(cat "$dir/compatible" 2>/dev/null || true)"

	if [ "$name" = radio ]; then
		echo "airport:Airport wireless"
		register-module airport
	elif [ "$name" = bmac ] || ([ "$device_type" = network ] && [ "$compatible" = bmac+ ]); then
		echo "bmac:PowerMac BMAC Ethernet"
		register-module bmac
	fi
done
