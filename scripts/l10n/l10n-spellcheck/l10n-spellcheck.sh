#!/bin/bash
#
# -*-sh-*-
#
#     Copyright (C) 2004-2005 Davide Viti <zinosat@tiscali.it>
# 
#     This program is free software; you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation; either version 2 of the License, or
#     (at your option) any later version.
# 
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public
#     License along with this program; if not, write to the Free
#     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#     MA 02111-1307 USA
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

# build "index.html" with the new results
export DIFF_DIR=${NEW}_$1
sh build_index.sh ${NEW}_$1/stats.txt ${HTML_PAGE} ${NEW}_$1/index.html

SAVED_STATS=`echo ${NEW}.txt | sed "s:check_:stats_:"`
cp ${NEW}_$1/stats.txt ${OUT_DIR}/history/${SAVED_STATS}
mv ${NEW}_$1/index.html ${OUT_DIR}

echo ""
echo "***  $SAVED_STATS  ***"

sh diff_stats.sh ${OUT_DIR}/latest/stats.txt ${NEW}_$1/stats.txt

LATEST=`ls -l ${OUT_DIR}/latest | sed "s:.*-> ::"`
rm ${OUT_DIR}/latest 
rm -fr $LATEST

mv ${NEW}_$1 ${OUT_DIR}/${NEW}
ln -s ${OUT_DIR}/${NEW}  ${OUT_DIR}/latest
