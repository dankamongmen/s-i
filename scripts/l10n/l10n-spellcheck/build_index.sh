# This script builds "index.html" starting from 
#
# "stats.txt"  (i.e.: 391 bg)
# "index_template.html" 

usage() {
    echo  "Usage:"
    echo  "$0 <stats file> <HTML template file> <output file>"
}

if [ -z "$1" ]
    then
    usage
    exit 1
fi

if [ -z "$2" ]
    then
    usage
    exit 1
fi

STATS=$1
TEMPLATE=$2
INDEX_HTML=$3
TABLE_HTML=stats.html
DICTIONARIES=build-tools/dictionaries.txt
# Compute some statistics
i=0
TOTAL=0
AVERAGE=0
for VAL in `cat $STATS | sort -n | awk '{print $1}'`; do
    TOTAL=`expr $TOTAL + $VAL`
    i=`expr $i + 1`
done

# avoid division by 0
if [ $i -ne 0 ] ; then
    AVERAGE=`expr $TOTAL / $i`
fi

# create HTML table
echo "<table>" > $TABLE_HTML
echo "  <tr>" >> $TABLE_HTML
echo "   <th class=\"col1\">Language</th>" >> $TABLE_HTML
echo "   <th class=\"t1\">Unknown words</th>" >> $TABLE_HTML
echo "   <th class=\"t1\">Messages</th>" >> $TABLE_HTML
echo "   <th class=\"t1\">List of unknown words</th>" >> $TABLE_HTML
echo "   <th class=\"t1\">Suspect variables</th>" >> $TABLE_HTML
echo "   <th class=\"t1\">All files</th>" >> $TABLE_HTML
echo "   <th class=\"t1\">Aspell Dictionary</th>" >> $TABLE_HTML
echo " </tr>" >> $TABLE_HTML

# fill the table:
# Language, Unknown words, Messages, List of unknown words, Suspect vars, All Files
for ROW in `cat $STATS | sed  "s: :,:g"`; do
    LANG=`echo $ROW | awk -F, '{print $2}'`
    UNKN=`echo $ROW | awk -F, '{print $1}'`
    SUSP=`echo $ROW | awk -F, '{print $3}'`

if [ $SUSP = "1" ] ; then
    SUSPECT_VARS_URL="latest/nozip/${LANG}_var.txt"
    SUSPECT_VARS_NAME="${LANG}_var.txt"
else
    SUSPECT_VARS_URL=""
    SUSPECT_VARS_NAME="-"
fi

if [ $LANG = "pt_BR" ] ; then
    DICT_URL=`grep -w "pt" $DICTIONARIES`
else
    DICT_URL=`grep -w "${LANG}" $DICTIONARIES`
fi

DICT_NAME=`echo $DICT_URL | sed "s|ftp://ftp.gnu.org/gnu/aspell/dict/.*/||" | sed "s:.tar.bz2$::"`

echo "  <tr>" >> $TABLE_HTML
echo "   <td class=\"col1\">$LANG</td>" >> $TABLE_HTML
echo "   <td>$UNKN</td>" >> $TABLE_HTML
echo "   <td><a href=\"latest/nozip/${LANG}_all.txt\">${LANG}_all.txt</a></td>" >> $TABLE_HTML
echo "   <td><a href=\"latest/nozip/${LANG}_unkn_wl.txt\">${LANG}_unkn_wl.txt</a></td>" >> $TABLE_HTML
echo "   <td><a href=\"$SUSPECT_VARS_URL\">$SUSPECT_VARS_NAME</a></td>" >> $TABLE_HTML
echo "   <td><a href=\"latest/zip/$LANG.tar.gz\">$LANG.tar.gz</a></td>" >> $TABLE_HTML
echo "   <td><a href=\"$DICT_URL\">$DICT_NAME</a></td>" >> $TABLE_HTML
echo "  </tr>" >> $TABLE_HTML
    
done

# Total unknown words
echo "  <tr>" >> $TABLE_HTML
echo "   <td class=\"col1\">TOTAL</td>" >> $TABLE_HTML
echo "   <td>$TOTAL</td>" >> $TABLE_HTML
echo "   <td></td>" >> $TABLE_HTML
echo "   <td></td>" >> $TABLE_HTML
echo "   <td></td>" >> $TABLE_HTML
echo "  </tr>" >> $TABLE_HTML

# Average unknown words per language
echo "  <tr>" >> $TABLE_HTML
echo "   <td class=\"col1\">AVERAGE</td>" >> $TABLE_HTML
echo "   <td>$AVERAGE</td>" >> $TABLE_HTML
echo "   <td></td>" >> $TABLE_HTML
echo "   <td></td>" >> $TABLE_HTML
echo "   <td></td>" >> $TABLE_HTML
echo "  </tr>" >> $TABLE_HTML

echo "</table>" >> $TABLE_HTML

sed "/<!-- HTML TABLE STARTS HERE -->/r ${TABLE_HTML}" $TEMPLATE > $INDEX_HTML

NOW="$(date --utc)"
sed "s|<\!-- TODAY DATE -->|$NOW|" $INDEX_HTML > temp.xxx
mv temp.xxx $INDEX_HTML 
rm $TABLE_HTML
