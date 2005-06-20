#!/bin/bash

# *** l10n-spellcheck.sh ***
export LOCAL_REPOSITORY="${HOME}/di_manual"
export REFRESH_CMD="svn up ${LOCAL_REPOSITORY}"
export OUT_DIR="/var/lib/gforge/chroot/home/groups/d-i/htdocs/spellcheck/manual_d-i/"

# *** check_dit.sh ***
export PO_FINDER="./pof_di-manual.sh"

# remove ${ALL_THESE_VARIABLES} which do not need to be spell checked
export REMOVE_VARS="no"
export ASPELL_EXTRA_PARAM="filter -H"

export PLOT_TITLE="Statistics for the debian-installer manual"

export HANDLE_SUSPECT_VARS="no"
