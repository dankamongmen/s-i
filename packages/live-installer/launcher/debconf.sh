#!/bin/sh

# The following debconf stuff needs to be in an own child. For some reason,
# debconf is messing with the FD and the environment, so that cdebconf in the
# installer is failing (will not react to any input).
#
# To avoid that, we're calling it in an own script. Calling it in a subshell
# did not work either.

set -e

. /usr/share/debconf/confmodule

db_version 2.0

db_input critical live-installer-launcher/mode || true
db_go

db_get live-installer-launcher/mode
MODE=$RET

FRONTEND=$(echo $MODE | awk -F- '{ print $1 }')
PRIORITY=$(echo $MODE | awk -F- '{ print $2 }')

db_fset live-installer-launcher/mode seen false
db_purge

# Write values to temporary file that can be sourced from the parent script.
echo "FRONTEND=$FRONTEND" > /tmp/live-installer
echo "PRIORITY=$PRIORITY" >> /tmp/live-installer
