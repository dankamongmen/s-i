# *** l10n-spellcheck.sh ***
export LOCAL_REPOSITORY="$HOME/debian-installer"
export OUT_DIR="$HOME/public_html/spellcheck/"

# *** check_all.sh ***
export LANGUAGE_LIST="./lang2dict.txt"
export HTML_PAGE="./d-i.html"

# *** check_dit.sh ***
export PO_FINDER="./po_finder.sh"

# remove ${ALL_THESE_VARIABLES} which do not need to be spell checked
export REMOVE_VARS="yes"
export ASPELL_EXTRA_PARAM=""

#export PLOT_TITLE=\"Statistics for the $i languages with an Aspell dictionary\"
export PLOT_TITLE="Statistics for the debian-installer"

export HANDLE_SUSPECT_VARS="yes"
