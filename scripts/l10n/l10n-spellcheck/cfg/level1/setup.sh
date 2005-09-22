#!/bin/bash

# *** l10n-spellcheck.sh ***
export LOCAL_REPOSITORY="${HOME}/d-i/levels/level1"
export REFRESH_CMD=""
export OUT_DIR="/var/lib/gforge/chroot/home/groups/d-i/htdocs/spellcheck/level1/"

# remove ${ALL_THESE_VARIABLES} which do not need to be spell checked
export REMOVE_VARS="yes"
export ASPELL_EXTRA_PARAM=

export PLOT_TITLE="Statistics for the level1"

export HANDLE_SUSPECT_VARS="yes"
