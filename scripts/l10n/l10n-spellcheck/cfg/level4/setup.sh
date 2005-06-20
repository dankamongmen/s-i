#!/bin/bash

# *** l10n-spellcheck.sh ***
export LOCAL_REPOSITORY="${HOME}/d-i/levels/level4"
export OUT_DIR="/var/lib/gforge/chroot/home/groups/d-i/htdocs/spellcheck/level4/"

# remove ${ALL_THESE_VARIABLES} which do not need to be spell checked
export REMOVE_VARS="yes"
export ASPELL_EXTRA_PARAM=

export PLOT_TITLE="Statistics for the level4"

export HANDLE_SUSPECT_VARS="yes"
