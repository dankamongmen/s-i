# Usage: pof_installer  <D-I repository path> <lang>
#
# This module selects a list of (po/pot) files for
# the debian installer (level 1)

DI_COPY=$1
LANG=$2

# look for a master file
if [ -f $DI_COPY/po/$LANG.po ] ; then
    echo "$DI_COPY/po/$LANG.po"
else # master file not available: look for sparse po files
    find $DI_COPY -name "$LANG.po"
fi


# English master file "template.pot" (before we found "en.po")
if [ $LANG = en ] ; then
    echo "$DI_COPY/po/template.pot"
fi
