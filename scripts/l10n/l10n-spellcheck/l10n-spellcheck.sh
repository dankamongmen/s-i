#!/bin/bash
#
# usage: l10n-spellcheck.sh
#
# l10n-spellcheck.sh - this script triggers all the spellchecking process
#
# Author: Davide Viti <zinosat@tiscali.it> 2005, for the Debian Project
#

export PATH=${HOME}/aspell:${HOME}/gnuplot/bin:${PATH}
SCRIPT_WITHOUT_PATH=`basename $0`
SCRIPTS_PATH=`echo $0 | sed "s:\(.*\)${SCRIPT_WITHOUT_PATH}\(.*\):\1\2:"`

. ${HOME}/${SCRIPTS_PATH}/cfg/$1/setup.sh
export LANGUAGE_LIST="cfg/$1/lang2dict.txt"
export WLS_PATH="./cfg/$1/wls"
export PO_FINDER="cfg/$1/po_finder.sh"
export HTML_PAGE="cfg/$1/report_page.html"

if  [ $? != 0 ] ; then
    echo "error: configuration file not found"
    exit 1
fi

NEW="check_$(date '+%Y%m%d')"

# update local copy of the repository (if necessary)
${REFRESH_CMD}


# spellcheck and move data to public_html
cd  ${HOME}/${SCRIPTS_PATH}
sh check_all.sh ${LOCAL_REPOSITORY} ${NEW}_$1
mv ${NEW}_$1 ${OUT_DIR}/${NEW}

# save symlink destinations
PREVIOUS=`ls -l ${OUT_DIR}/previous | sed "s:.*-> ::"`
LATEST=`ls -l ${OUT_DIR}/latest | sed "s:.*-> ::"`
SAVED_STATS=`echo ${NEW}.txt | sed "s:check_:stats_:"`

rm ${OUT_DIR}/previous
rm ${OUT_DIR}/latest

ln -s ${OUT_DIR}/${NEW}    ${OUT_DIR}/latest
ln -s ${LATEST}          ${OUT_DIR}/previous

# purge old entry
rm -fr ${PREVIOUS}

cp ${OUT_DIR}/latest/stats.txt ${OUT_DIR}/history/${SAVED_STATS}
mv ${OUT_DIR}/latest/index.html ${OUT_DIR}

echo ""
echo "***  $SAVED_STATS  ***"

sh diff_stats.sh ${OUT_DIR}/previous/stats.txt ${OUT_DIR}/latest/stats.txt
