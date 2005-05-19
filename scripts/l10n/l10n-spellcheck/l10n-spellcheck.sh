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

SAVED_STATS=`echo ${NEW}.txt | sed "s:check_:stats_:"`
cp ${NEW}_$1/stats.txt ${OUT_DIR}/history/${SAVED_STATS}
mv ${NEW}_$1/index.html ${OUT_DIR}

OLD_PATH=${OUT_DIR}/latest/nozip
NEW_PATH=${NEW}_$1/nozip

for ROW in `cat ${NEW}_$1/stats.txt | sed 's: :,:g'`; do
    VAL=`echo ${ROW}| awk -F, '{print $1}'`
    LANG=`echo ${ROW}| awk -F, '{print $2}'`

    LALL=${LANG}_all.txt
    LUWL=${LANG}_unkn_wl.txt
    LVAR=${LANG}_var.txt
  
    for TO_DIFF in ${LALL} ${LUWL} ${LVAR} ; do
	if [ -f ${OLD_PATH}/${TO_DIFF} -o -f ${NEW_PATH}/${TO_DIFF} ] ; then

	    diff -u0 -N \
		--label old ${OLD_PATH}/${TO_DIFF} \
		--label new ${NEW_PATH}/${TO_DIFF} > \
		${NEW_PATH}/${TO_DIFF/.txt/.diff}
	fi
    done

done

# remove empty files
find ${NEW_PATH} -empty -exec rm '{}' ';'

echo ""
echo "***  $SAVED_STATS  ***"

sh diff_stats.sh ${OUT_DIR}/latest/stats.txt ${NEW}_$1/stats.txt

LATEST=`ls -l ${OUT_DIR}/latest | sed "s:.*-> ::"`
rm ${OUT_DIR}/latest 
rm -fr $LATEST

mv ${NEW}_$1 ${OUT_DIR}/${NEW}
ln -s ${OUT_DIR}/${NEW}  ${OUT_DIR}/latest
