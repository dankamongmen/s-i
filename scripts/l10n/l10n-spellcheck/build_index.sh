# This script builds "index.html" starting from 
#
# "stats.txt"  (i.e.: 391 bg)
# "index_template.html" 

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
TABLE_HTML=stats.html_${RANDOM}
DICTIONARIES=build-tools/dictionaries.txt

# Compute some statistics
i=0
TOTAL=0
AVERAGE=0
for VAL in `cat ${STATS} | sort -n | awk '{print $1}'`; do
    if [ ${VAL} -ne -1 ] ; then
	TOTAL=`expr ${TOTAL} + ${VAL}`
	i=`expr $i + 1`
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
echo "   <th class=\"t1\">All files</th>"
echo "   <th class=\"t1\">Aspell Dictionary</th>"
echo " </tr>"

# fill the table:
# Language, Unknown words, Messages, List of unknown words, Suspect vars, All Files
for ROW in `cat ${STATS} | sed  "s: :,:g"`; do
    LANG=`echo ${ROW} | awk -F, '{print $2}'`
    UNKN=`echo ${ROW} | awk -F, '{print $1}'`
    
    ISO_CODE=`grep -w "${LANG}" iso_codes.txt | awk -F, '{print $2}'`

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

if [ ${LANG} = "pt_BR" ] ; then
    DICT_URL=`grep -w "pt" ${DICTIONARIES}`
else
    DICT_URL=`grep -w "${LANG}" ${DICTIONARIES}`
fi

DICT_NAME=`echo ${DICT_URL} | sed "s|ftp://ftp.gnu.org/gnu/aspell/dict/.*/||" | sed "s:.tar.bz2$::"`

echo "  <tr>"
echo "   <td class=\"col1\">${ISO_CODE} [${LANG}]</td>"

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

if [ ${UNKN} -eq -1 ] ; then
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
echo "   <td><a href=\"latest/zip/${LANG}.tar.gz\">${LANG}.tar.gz</a></td>"
echo "   <td><a href=\"${DICT_URL}\">${DICT_NAME}</a></td>"
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

sed "/<!-- HTML TABLE STARTS HERE -->/r ${TABLE_HTML}" ${TEMPLATE} > ${INDEX_HTML}

RAND_EXT=${RANDOM}

NOW="$(date --utc)"
sed "s|<\!-- TODAY DATE -->|${NOW}|" ${INDEX_HTML} > temp.${RAND_EXT}
mv temp.${RAND_EXT} ${INDEX_HTML}

sed "s|<!-- COMMON WL -->|latest/nozip/di_common_wl.txt|" ${INDEX_HTML} > temp.${RAND_EXT}
mv temp.${RAND_EXT} ${INDEX_HTML} 

rm ${TABLE_HTML}
