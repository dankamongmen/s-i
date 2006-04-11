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

usage() {
    echo  "Usage:"
    echo  "$0 <stats file> <HTML template file> <output file>"
}

if [ -z "$1" ] ; then
    usage
    exit 1
fi

if [ -z "$2" ] ; then
    usage
    exit 1
fi

STATS=$1
TEMPLATE=$2
INDEX_HTML=$3
TABLE_HTML=`mktemp`

# Compute some statistics
i=0
TOTAL=0
AVERAGE=0
UNIQUE_CODEPOINTS=0
for VAL in `sort -n ${STATS} | awk '{print $1}'`; do
    if [ ${VAL} -ne -1 ] ; then
	TOTAL=`expr ${TOTAL} + ${VAL}`
	i=$((i+1))
    fi
done

# avoid division by 0
if [ $i -ne 0 ] ; then
    AVERAGE=`expr ${TOTAL} / $i`
fi

LOGFILE=${TABLE_HTML}
exec 6>&1           # Link file descriptor #6 with stdout.
                    # Saves stdout.

exec > ${LOGFILE}     # stdout replaced with file "logfile.txt".

# create HTML table
echo "<table>"
echo "  <tr>"
echo "   <th class=\"col1\">Language</th>"
echo "   <th class=\"t1\">Unknown words</th>"
echo "   <th class=\"t1\">Messages</th>"
echo "   <th class=\"t1\">List of unknown words</th>"

if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
    echo "   <th class=\"t1\">Suspect variables</th>"
fi

echo "   <th class=\"t1\">Custom wordlist</th>"
echo "   <th class=\"t1\">Codepoints</th>"
echo "   <th class=\"t1\">All files</th>"
echo " </tr>"

# fill the table:
# Lang, Unkn words, Msg, List of unknown words, Susp vars, Custom wl, codepoints, All Files
for ROW in `sed  "s: :,:g" ${STATS}`; do
    LANG=`echo ${ROW} | awk -F, '{print $2}'`
    UNKN=`echo ${ROW} | awk -F, '{print $1}'`
    CODEPOINTS=`echo ${ROW} | awk -F, '{print $4}'`

    ISO_CODE=`grep -w "^${LANG}" ${CFG_DIR}/iso_codes.txt | awk -F, '{print $2}'`

if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
    SUSP=`echo ${ROW} | awk -F, '{print $3}'`

    if [ ${SUSP} = "1" ] ; then
	SUSPECT_VARS_URL="latest/nozip/${LANG}_var.txt"
	SUSPECT_VARS_NAME="${LANG}_var.txt"
    else
	SUSPECT_VARS_URL=""
	SUSPECT_VARS_NAME="-"
    fi
fi

if [ -f ${WLS_PATH}/${LANG}_wl.txt ] ; then
    WORDLIST="latest/nozip/${LANG}_wl.txt"
    WORDLIST_NAME="${LANG}_wl.txt"
else
    WORDLIST=""
    WORDLIST_NAME="-"
fi

echo "  <tr>"
echo "   <td class=\"col1\">${ISO_CODE} [${LANG}]</td>"

# Number of unknown words
if [ ${UNKN} -eq -1 ] ; then
    echo "   <td>-</td>"
else
    echo "   <td>${UNKN}</td>"
fi

if [ -f ${DIFF_DIR}/nozip/${LANG}_all.diff ] ; then
    DIFF="<a href=\"latest/nozip/${LANG}_all.diff\">  (*)</a>"
else
    DIFF=
fi

echo "   <td><a href=\"latest/nozip/${LANG}_all.txt\">${LANG}_all.txt</a>${DIFF}</td>"

# List of unknown words
if [ ${UNKN} -eq -1 ] || [ ${UNKN} -eq 0 ] ; then
    echo "   <td>-</td>"
else

    if [ -f ${DIFF_DIR}/nozip/${LANG}_unkn_wl.diff ] ; then
	DIFF="<a href=\"latest/nozip/${LANG}_unkn_wl.diff\">  (*)</a>"
    else
	DIFF=
    fi

    echo "   <td><a href=\"latest/nozip/${LANG}_unkn_wl.txt\">${LANG}_unkn_wl.txt</a>${DIFF}</td>"
fi

if [ ${HANDLE_SUSPECT_VARS} = "yes" ] ; then
    SUSP_PATH=${SUSPECT_VARS_URL/latest/}
    if [ -f ${DIFF_DIR}/${SUSP_PATH/.txt/.diff} ] ; then
	DIFF="<a href=\"${SUSPECT_VARS_URL/.txt/.diff}\">  (*)</a>"
    else
	DIFF=
    fi

    echo "   <td><a href=\"${SUSPECT_VARS_URL}\">${SUSPECT_VARS_NAME}</a>${DIFF}</td>"
fi

echo "   <td><a href=\"${WORDLIST}\">${WORDLIST_NAME}</a></td>"

if [ ${CODEPOINTS} -eq 0 ] ; then
    echo "   <td>-</td>"
else
    echo "   <td><a href=\"latest/nozip/${LANG}_codes.txt\">${CODEPOINTS}</a></td>"
fi

echo "   <td><a href=\"latest/zip/${LANG}.tar.gz\">${LANG}.tar.gz</a></td>"
echo "  </tr>"
    
done

# Total unknown words
echo "  <tr>"
echo "   <td class=\"col1\">TOTAL</td>"
echo "   <td>${TOTAL}</td>"
echo "   <td></td>"
echo "   <td></td>"
echo "   <td></td>"
echo "  </tr>"

# Average unknown words per language
echo "  <tr>"
echo "   <td class=\"col1\">AVERAGE</td>"
echo "   <td>${AVERAGE}</td>"
echo "   <td></td>"
echo "   <td></td>"
echo "   <td></td>"
echo "  </tr>"

echo "</table>"
exec 1>&6 6>&-      # Restore stdout and close file descriptor #6.

sed -e "/<!-- HTML TABLE STARTS HERE -->/r ${TABLE_HTML}" \
    -e "s|<!-- TODAY DATE -->|$(date --utc)|" \
    -e "s|<!-- COMMON WL -->|latest/nozip/di_common_wl.txt|" \
    -e "s|<!-- CODEPOINTS_TARBALL -->|latest/zip/codepoints.tar.gz|" \
    -e "s|<!-- CODEPOINTS -->|latest/all_codes.txt|" \
    ${TEMPLATE} > ${INDEX_HTML}
